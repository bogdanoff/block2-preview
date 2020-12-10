
/*
 * block2: Efficient MPO implementation of quantum chemistry DMRG
 * Copyright (C) 2020 Huanchen Zhai <hczhai@caltech.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "parallel_rule.hpp"
#include "tensor_functions.hpp"
#include <cassert>
#include <map>
#include <memory>
#include <set>

using namespace std;

namespace block2 {

// Operations for operator tensors (distributed parallel)
template <typename S> struct ParallelTensorFunctions : TensorFunctions<S> {
    using TensorFunctions<S>::opf;
    using TensorFunctions<S>::parallel_for;
    shared_ptr<ParallelRule<S>> rule;
    ParallelTensorFunctions(const shared_ptr<OperatorFunctions<S>> &opf,
                            const shared_ptr<ParallelRule<S>> &rule)
        : TensorFunctions<S>(opf), rule(rule) {}
    shared_ptr<TensorFunctions<S>> copy() const override {
        return make_shared<ParallelTensorFunctions<S>>(opf->copy(), rule);
    }
    // c = a
    void left_assign(const shared_ptr<OperatorTensor<S>> &a,
                     shared_ptr<OperatorTensor<S>> &c) const override {
        assert(a->lmat != nullptr);
        assert(a->lmat->get_type() == SymTypes::RVec);
        assert(c->lmat != nullptr);
        assert(c->lmat->get_type() == SymTypes::RVec);
        assert(a->lmat->data.size() == c->lmat->data.size());
        vector<size_t> idxs;
        idxs.reserve(a->lmat->data.size());
        for (size_t i = 0; i < a->lmat->data.size(); i++) {
            if (a->lmat->data[i]->get_type() == OpTypes::Zero)
                c->lmat->data[i] = a->lmat->data[i];
            else {
                assert(a->lmat->data[i] == c->lmat->data[i]);
                auto pa = abs_value(a->lmat->data[i]),
                     pc = abs_value(c->lmat->data[i]);
                if (rule->available(pc)) {
                    assert(rule->available(pa));
                    assert(c->ops[pc]->data == nullptr);
                    if (frame->use_main_stack)
                        c->ops[pc]->allocate(c->ops[pc]->info);
                    idxs.push_back(i);
                    c->ops[pc]->factor = a->ops[pa]->factor;
                }
            }
        }
        parallel_for(idxs.size(),
                     [&a, &c, &idxs](const shared_ptr<TensorFunctions<S>> &tf,
                                     size_t ii) {
                         size_t i = idxs[ii];
                         auto pa = abs_value(a->lmat->data[i]),
                              pc = abs_value(c->lmat->data[i]);
                         if (!frame->use_main_stack) {
                             c->ops[pc]->alloc =
                                 make_shared<VectorAllocator<double>>();
                             c->ops[pc]->allocate(c->ops[pc]->info);
                         }
                         if (c->ops[pc]->info->n == a->ops[pa]->info->n)
                             c->ops[pc]->copy_data_from(a->ops[pa], true);
                         else
                             c->ops[pc]->selective_copy_from(a->ops[pa], true);
                     });
    }
    // c = a
    void right_assign(const shared_ptr<OperatorTensor<S>> &a,
                      shared_ptr<OperatorTensor<S>> &c) const override {
        assert(a->rmat != nullptr);
        assert(a->rmat->get_type() == SymTypes::CVec);
        assert(c->rmat != nullptr);
        assert(c->rmat->get_type() == SymTypes::CVec);
        assert(a->rmat->data.size() == c->rmat->data.size());
        vector<size_t> idxs;
        idxs.reserve(a->lmat->data.size());
        for (size_t i = 0; i < a->rmat->data.size(); i++) {
            if (a->rmat->data[i]->get_type() == OpTypes::Zero)
                c->rmat->data[i] = a->rmat->data[i];
            else {
                assert(a->rmat->data[i] == c->rmat->data[i]);
                auto pa = abs_value(a->rmat->data[i]),
                     pc = abs_value(c->rmat->data[i]);
                if (rule->available(pc)) {
                    assert(rule->available(pa));
                    assert(c->ops[pc]->data == nullptr);
                    if (frame->use_main_stack)
                        c->ops[pc]->allocate(c->ops[pc]->info);
                    idxs.push_back(i);
                    c->ops[pc]->factor = a->ops[pa]->factor;
                }
            }
        }
        parallel_for(idxs.size(),
                     [&a, &c, &idxs](const shared_ptr<TensorFunctions<S>> &tf,
                                     size_t ii) {
                         size_t i = idxs[ii];
                         auto pa = abs_value(a->rmat->data[i]),
                              pc = abs_value(c->rmat->data[i]);
                         if (!frame->use_main_stack) {
                             c->ops[pc]->alloc =
                                 make_shared<VectorAllocator<double>>();
                             c->ops[pc]->allocate(c->ops[pc]->info);
                         }
                         if (c->ops[pc]->info->n == a->ops[pa]->info->n)
                             c->ops[pc]->copy_data_from(a->ops[pa], true);
                         else
                             c->ops[pc]->selective_copy_from(a->ops[pa], true);
                     });
    }
    // vmat = expr[L part | R part] x cmat (for perturbative noise)
    void tensor_product_partial_multiply(
        const shared_ptr<OpExpr<S>> &expr,
        const shared_ptr<OperatorTensor<S>> &lopt,
        const shared_ptr<OperatorTensor<S>> &ropt, bool trace_right,
        const shared_ptr<SparseMatrix<S>> &cmat,
        const vector<pair<uint8_t, S>> &psubsl,
        const vector<
            vector<shared_ptr<typename SparseMatrixInfo<S>::ConnectionInfo>>>
            &cinfos,
        const vector<S> &vdqs, const shared_ptr<SparseMatrixGroup<S>> &vmats,
        int &vidx, bool do_reduce) const override {
        if (expr->get_type() == OpTypes::ExprRef) {
            shared_ptr<OpExprRef<S>> op =
                dynamic_pointer_cast<OpExprRef<S>>(expr);
            TensorFunctions<S>::tensor_product_partial_multiply(
                op->op, lopt, ropt, trace_right, cmat, psubsl, cinfos, vdqs,
                vmats, vidx, false);
            if (opf->seq->mode != SeqTypes::Auto && do_reduce)
                rule->comm->reduce_sum(vmats, rule->comm->root);
        } else
            TensorFunctions<S>::tensor_product_partial_multiply(
                expr, lopt, ropt, trace_right, cmat, psubsl, cinfos, vdqs,
                vmats, vidx, false);
    }
    // vmats = expr x cmats
    void
    tensor_product_multi_multiply(const shared_ptr<OpExpr<S>> &expr,
                                  const shared_ptr<OperatorTensor<S>> &lopt,
                                  const shared_ptr<OperatorTensor<S>> &ropt,
                                  const shared_ptr<SparseMatrixGroup<S>> &cmats,
                                  const shared_ptr<SparseMatrixGroup<S>> &vmats,
                                  S opdq, bool all_reduce) const override {
        if (expr->get_type() == OpTypes::ExprRef) {
            shared_ptr<OpExprRef<S>> op =
                dynamic_pointer_cast<OpExprRef<S>>(expr);
            TensorFunctions<S>::tensor_product_multi_multiply(
                op->op, lopt, ropt, cmats, vmats, opdq, false);
            if (all_reduce)
                rule->comm->allreduce_sum(vmats);
        } else
            TensorFunctions<S>::tensor_product_multi_multiply(
                expr, lopt, ropt, cmats, vmats, opdq, false);
    }
    // vmat = expr x cmat
    void tensor_product_multiply(const shared_ptr<OpExpr<S>> &expr,
                                 const shared_ptr<OperatorTensor<S>> &lopt,
                                 const shared_ptr<OperatorTensor<S>> &ropt,
                                 const shared_ptr<SparseMatrix<S>> &cmat,
                                 const shared_ptr<SparseMatrix<S>> &vmat,
                                 S opdq, bool all_reduce) const override {
        if (expr->get_type() == OpTypes::ExprRef) {
            shared_ptr<OpExprRef<S>> op =
                dynamic_pointer_cast<OpExprRef<S>>(expr);
            TensorFunctions<S>::tensor_product_multiply(
                op->op, lopt, ropt, cmat, vmat, opdq, false);
            if (all_reduce)
                rule->comm->allreduce_sum(vmat);
        } else
            TensorFunctions<S>::tensor_product_multiply(expr, lopt, ropt, cmat,
                                                        vmat, opdq, false);
    }
    // mat = diag(expr)
    void tensor_product_diagonal(const shared_ptr<OpExpr<S>> &expr,
                                 const shared_ptr<OperatorTensor<S>> &lopt,
                                 const shared_ptr<OperatorTensor<S>> &ropt,
                                 const shared_ptr<SparseMatrix<S>> &mat,
                                 S opdq) const override {
        if (expr->get_type() == OpTypes::ExprRef) {
            shared_ptr<OpExprRef<S>> op =
                dynamic_pointer_cast<OpExprRef<S>>(expr);
            TensorFunctions<S>::tensor_product_diagonal(op->op, lopt, ropt, mat,
                                                        opdq);
            if (opf->seq->mode != SeqTypes::Auto)
                rule->comm->allreduce_sum(mat);
        } else
            TensorFunctions<S>::tensor_product_diagonal(expr, lopt, ropt, mat,
                                                        opdq);
    }
    // c = mpst_bra x a x mpst_ket
    void left_rotate(const shared_ptr<OperatorTensor<S>> &a,
                     const shared_ptr<SparseMatrix<S>> &mpst_bra,
                     const shared_ptr<SparseMatrix<S>> &mpst_ket,
                     shared_ptr<OperatorTensor<S>> &c) const override {
        for (size_t i = 0; i < a->lmat->data.size(); i++)
            if (a->lmat->data[i]->get_type() != OpTypes::Zero) {
                auto pa = abs_value(a->lmat->data[i]);
                if (rule->available(pa)) {
                    assert(c->ops.at(pa)->data == nullptr);
                    c->ops.at(pa)->allocate(c->ops.at(pa)->info);
                }
            }
        bool repeat = true, no_repeat = rule->non_blocking ? false : true;
        auto f = [&a, &c, &mpst_bra, &mpst_ket, this, &repeat, &no_repeat](
                     const shared_ptr<TensorFunctions<S>> &tf, size_t i) {
            if (a->lmat->data[i]->get_type() != OpTypes::Zero) {
                auto pa = abs_value(a->lmat->data[i]);
                if (this->rule->own(pa) &&
                    ((repeat && this->rule->repeat(pa)) ||
                     (no_repeat && !this->rule->repeat(pa))))
                    tf->opf->tensor_rotate(a->ops.at(pa), c->ops.at(pa),
                                           mpst_bra, mpst_ket, false);
            }
        };
        parallel_for(a->lmat->data.size(), f);
        if (opf->seq->mode == SeqTypes::Auto)
            opf->seq->auto_perform();
        for (size_t i = 0; i < a->lmat->data.size(); i++)
            if (a->lmat->data[i]->get_type() != OpTypes::Zero) {
                auto pa = abs_value(a->lmat->data[i]);
                if (rule->repeat(pa)) {
                    if (!rule->non_blocking)
                        rule->comm->broadcast(c->ops.at(pa), rule->owner(pa));
                    else
                        rule->comm->ibroadcast(c->ops.at(pa), rule->owner(pa));
                }
            }
        if (rule->non_blocking) {
            repeat = false, no_repeat = true;
            parallel_for(a->lmat->data.size(), f);
            if (opf->seq->mode == SeqTypes::Auto)
                opf->seq->auto_perform();
            rule->comm->waitall();
        }
    }
    // c = mpst_bra x a x mpst_ket
    void right_rotate(const shared_ptr<OperatorTensor<S>> &a,
                      const shared_ptr<SparseMatrix<S>> &mpst_bra,
                      const shared_ptr<SparseMatrix<S>> &mpst_ket,
                      shared_ptr<OperatorTensor<S>> &c) const override {
        for (size_t i = 0; i < a->rmat->data.size(); i++)
            if (a->rmat->data[i]->get_type() != OpTypes::Zero) {
                auto pa = abs_value(a->rmat->data[i]);
                if (rule->available(pa)) {
                    assert(c->ops.at(pa)->data == nullptr);
                    c->ops.at(pa)->allocate(c->ops.at(pa)->info);
                }
            }
        bool repeat = true, no_repeat = rule->non_blocking ? false : true;
        auto f = [&a, &c, &mpst_bra, &mpst_ket, this, &repeat, &no_repeat](
                     const shared_ptr<TensorFunctions<S>> &tf, size_t i) {
            if (a->rmat->data[i]->get_type() != OpTypes::Zero) {
                auto pa = abs_value(a->rmat->data[i]);
                if (this->rule->own(pa) &&
                    ((repeat && this->rule->repeat(pa)) ||
                     (no_repeat && !this->rule->repeat(pa))))
                    tf->opf->tensor_rotate(a->ops.at(pa), c->ops.at(pa),
                                           mpst_bra, mpst_ket, true);
            }
        };
        parallel_for(a->rmat->data.size(), f);
        if (opf->seq->mode == SeqTypes::Auto)
            opf->seq->auto_perform();
        for (size_t i = 0; i < a->rmat->data.size(); i++)
            if (a->rmat->data[i]->get_type() != OpTypes::Zero) {
                auto pa = abs_value(a->rmat->data[i]);
                if (rule->repeat(pa)) {
                    if (!rule->non_blocking)
                        rule->comm->broadcast(c->ops.at(pa), rule->owner(pa));
                    else
                        rule->comm->ibroadcast(c->ops.at(pa), rule->owner(pa));
                }
            }
        if (rule->non_blocking) {
            repeat = false, no_repeat = true;
            parallel_for(a->rmat->data.size(), f);
            if (opf->seq->mode == SeqTypes::Auto)
                opf->seq->auto_perform();
            rule->comm->waitall();
        }
    }
    void intermediates(const shared_ptr<Symbolic<S>> &names,
                       const shared_ptr<Symbolic<S>> &exprs,
                       const shared_ptr<OperatorTensor<S>> &a,
                       bool left) const override {
        auto f = [&names, &a, left,
                  this](const vector<shared_ptr<OpExpr<S>>> &local_exprs) {
            shared_ptr<Symbolic<S>> ex = names->copy();
            ex->data = local_exprs;
            this->TensorFunctions<S>::intermediates(names, ex, a, left);
        };
        vector<shared_ptr<SparseMatrix<S>>> mats;
        rule->distributed_apply(f, names->data, exprs->data, mats);
    }
    // Numerical transform from normal operators
    // to complementary operators near the middle site
    void
    numerical_transform(const shared_ptr<OperatorTensor<S>> &a,
                        const shared_ptr<Symbolic<S>> &names,
                        const shared_ptr<Symbolic<S>> &exprs) const override {
        for (auto &op : a->ops)
            if (op.second->data == nullptr)
                op.second->allocate(op.second->info);
        assert(names->data.size() == exprs->data.size());
        assert((a->lmat == nullptr) ^ (a->rmat == nullptr));
        if (a->lmat == nullptr)
            a->rmat = names;
        else
            a->lmat = names;
        vector<pair<shared_ptr<SparseMatrix<S>>, shared_ptr<OpSum<S>>>> trs;
        trs.reserve(names->data.size());
        int maxi = 0;
        for (size_t k = 0; k < names->data.size(); k++) {
            if (exprs->data[k]->get_type() == OpTypes::Zero)
                continue;
            shared_ptr<OpExpr<S>> nop = abs_value(names->data[k]);
            shared_ptr<OpExpr<S>> expr =
                exprs->data[k] *
                (1 /
                 dynamic_pointer_cast<OpElement<S>>(names->data[k])->factor);
            if (expr->get_type() != OpTypes::ExprRef)
                expr = rule->localize_expr(expr, rule->owner(nop))->op;
            else
                expr = dynamic_pointer_cast<OpExprRef<S>>(expr)->op;
            assert(a->ops.count(nop) != 0);
            shared_ptr<SparseMatrix<S>> anop = a->ops.at(nop);
            switch (expr->get_type()) {
            case OpTypes::Sum:
                trs.push_back(
                    make_pair(anop, dynamic_pointer_cast<OpSum<S>>(expr)));
                maxi = max(
                    maxi,
                    (int)dynamic_pointer_cast<OpSum<S>>(expr)->strings.size());
                break;
            case OpTypes::Zero:
                break;
            default:
                assert(false);
                break;
            }
        }
        parallel_for(
            trs.size(),
            [&trs, &a](const shared_ptr<TensorFunctions<S>> &tf, size_t i) {
                shared_ptr<OpSum<S>> op = trs[i].second;
                for (size_t j = 0; j < op->strings.size(); j++) {
                    shared_ptr<OpElement<S>> nexpr = op->strings[j]->get_op();
                    assert(a->ops.count(nexpr) != 0);
                    tf->opf->iadd(trs[i].first, a->ops.at(nexpr),
                                  op->strings[j]->factor,
                                  op->strings[j]->conj != 0);
                    if (tf->opf->seq->mode == SeqTypes::Simple)
                        tf->opf->seq->simple_perform();
                }
            });
        if (opf->seq->mode == SeqTypes::Auto)
            opf->seq->auto_perform();
        for (size_t k = 0; k < names->data.size(); k++) {
            shared_ptr<OpExpr<S>> nop = abs_value(names->data[k]);
            if (exprs->data[k]->get_type() == OpTypes::Zero)
                continue;
            shared_ptr<OpExpr<S>> expr = exprs->data[k];
            bool is_local = false;
            if (expr->get_type() != OpTypes::ExprRef)
                is_local =
                    rule->localize_expr(expr, rule->owner(nop))->is_local;
            else
                is_local = dynamic_pointer_cast<OpExprRef<S>>(expr)->is_local;
            if (!is_local)
                rule->comm->reduce_sum(a->ops.at(nop), rule->owner(nop));
        }
    }
    // delayed left and right block contraction
    shared_ptr<DelayedOperatorTensor<S>>
    delayed_contract(const shared_ptr<OperatorTensor<S>> &a,
                     const shared_ptr<OperatorTensor<S>> &b,
                     const shared_ptr<OpExpr<S>> &op,
                     OpNamesSet delayed) const override {
        shared_ptr<DelayedOperatorTensor<S>> dopt =
            TensorFunctions<S>::delayed_contract(a, b, op, delayed);
        bool dleft = a->get_type() == OperatorTensorTypes::Delayed;
        dopt->mat->data[0] = rule->localize_expr(
            dopt->mat->data[0], rule->owner(dopt->dops[0]), dleft);
        return dopt;
    }
    // delayed left and right block contraction
    // using the pre-computed exprs
    shared_ptr<DelayedOperatorTensor<S>>
    delayed_contract(const shared_ptr<OperatorTensor<S>> &a,
                     const shared_ptr<OperatorTensor<S>> &b,
                     const shared_ptr<Symbolic<S>> &ops,
                     const shared_ptr<Symbolic<S>> &exprs,
                     OpNamesSet delayed) const override {
        shared_ptr<DelayedOperatorTensor<S>> dopt =
            TensorFunctions<S>::delayed_contract(a, b, ops, exprs, delayed);
        bool dleft = a->get_type() == OperatorTensorTypes::Delayed;
        for (size_t i = 0; i < dopt->mat->data.size(); i++)
            if (dopt->mat->data[i]->get_type() != OpTypes::ExprRef)
                dopt->mat->data[i] = rule->localize_expr(
                    dopt->mat->data[i], rule->owner(dopt->dops[i]), dleft);
        return dopt;
    }
    // c = a x b (dot)
    void left_contract(const shared_ptr<OperatorTensor<S>> &a,
                       const shared_ptr<OperatorTensor<S>> &b,
                       shared_ptr<OperatorTensor<S>> &c,
                       const shared_ptr<Symbolic<S>> &cexprs = nullptr,
                       OpNamesSet delayed = OpNamesSet()) const override {
        if (a == nullptr)
            left_assign(b, c);
        else {
            shared_ptr<Symbolic<S>> exprs =
                cexprs == nullptr ? a->lmat * b->lmat : cexprs;
            assert(exprs->data.size() == c->lmat->data.size());
            vector<shared_ptr<SparseMatrix<S>>> mats(exprs->data.size());
            for (size_t i = 0; i < exprs->data.size(); i++) {
                shared_ptr<OpExpr<S>> op = abs_value(c->lmat->data[i]);
                if (!delayed(dynamic_pointer_cast<OpElement<S>>(op)->name))
                    mats[i] = c->ops.at(op);
            }
            auto f = [&a, &b, &mats,
                      this](const vector<shared_ptr<OpExpr<S>>> &local_exprs) {
                for (size_t i = 0; i < local_exprs.size(); i++)
                    if (frame->use_main_stack && local_exprs[i] != nullptr) {
                        assert(mats[i]->data == nullptr);
                        mats[i]->allocate(mats[i]->info);
                    }
                this->parallel_for(
                    local_exprs.size(),
                    [&a, &b, &mats, &local_exprs](
                        const shared_ptr<TensorFunctions<S>> &tf, size_t i) {
                        if (local_exprs[i] != nullptr) {
                            if (!frame->use_main_stack) {
                                mats[i]->alloc =
                                    make_shared<VectorAllocator<double>>();
                                mats[i]->allocate(mats[i]->info);
                            }
                            tf->tensor_product(local_exprs[i], a->ops, b->ops,
                                               mats[i]);
                        }
                    });
                if (this->opf->seq->mode == SeqTypes::Auto)
                    this->opf->seq->auto_perform();
            };
            rule->distributed_apply(f, c->lmat->data, exprs->data, mats);
        }
    }
    // c = b (dot) x a
    void right_contract(const shared_ptr<OperatorTensor<S>> &a,
                        const shared_ptr<OperatorTensor<S>> &b,
                        shared_ptr<OperatorTensor<S>> &c,
                        const shared_ptr<Symbolic<S>> &cexprs = nullptr,
                        OpNamesSet delayed = OpNamesSet()) const override {
        if (a == nullptr)
            right_assign(b, c);
        else {
            shared_ptr<Symbolic<S>> exprs =
                cexprs == nullptr ? b->rmat * a->rmat : cexprs;
            assert(exprs->data.size() == c->rmat->data.size());
            vector<shared_ptr<SparseMatrix<S>>> mats(exprs->data.size());
            for (size_t i = 0; i < exprs->data.size(); i++) {
                shared_ptr<OpExpr<S>> op = abs_value(c->rmat->data[i]);
                if (!delayed(dynamic_pointer_cast<OpElement<S>>(op)->name))
                    mats[i] = c->ops.at(op);
            }
            auto f = [&a, &b, &mats,
                      this](const vector<shared_ptr<OpExpr<S>>> &local_exprs) {
                for (size_t i = 0; i < local_exprs.size(); i++)
                    if (frame->use_main_stack && local_exprs[i] != nullptr) {
                        assert(mats[i]->data == nullptr);
                        mats[i]->allocate(mats[i]->info);
                    }
                this->parallel_for(
                    local_exprs.size(),
                    [&a, &b, &mats, &local_exprs](
                        const shared_ptr<TensorFunctions<S>> &tf, size_t i) {
                        if (local_exprs[i] != nullptr) {
                            if (!frame->use_main_stack) {
                                mats[i]->alloc =
                                    make_shared<VectorAllocator<double>>();
                                mats[i]->allocate(mats[i]->info);
                            }
                            tf->tensor_product(local_exprs[i], b->ops, a->ops,
                                               mats[i]);
                        }
                    });
                if (this->opf->seq->mode == SeqTypes::Auto)
                    this->opf->seq->auto_perform();
            };
            rule->distributed_apply(f, c->rmat->data, exprs->data, mats);
        }
    }
};

} // namespace block2
