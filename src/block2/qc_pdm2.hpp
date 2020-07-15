
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

#include "expr.hpp"
#include "hamiltonian.hpp"
#include "mpo.hpp"
#include "operator_tensor.hpp"
#include "symbolic.hpp"
#include "tensor_functions.hpp"
#include <cassert>
#include <memory>

using namespace std;

#define SI(s) ((s)&1)
#define SJ(s) (((s)&2) >> 1)
#define SK(s) (((s)&4) >> 2)
#define SL(s) (((s)&8) >> 3)
#define PI(s) (s)
#define PJ(s) ((s) << 1)
#define PK(s) ((s) << 2)
#define PL(s) ((s) << 3)
#define PIJ(si, sj) (PI(si) | PJ(sj))
#define PIJK(si, sj, sk) (PI(si) | PJ(sj) | PK(sk))

namespace block2 {

template <typename, typename = void> struct PDM2MPOQC;

// "MPO" for two particle density matrix (non-spin-adapted)
template <typename S> struct PDM2MPOQC<S, typename S::is_sz_t> : MPO<S> {
    PDM2MPOQC(const Hamiltonian<S> &hamil) : MPO<S>(hamil.n_sites) {
        const auto n_sites = MPO<S>::n_sites;
        shared_ptr<OpExpr<S>> i_op =
            make_shared<OpElement<S>>(OpNames::I, SiteIndex(), hamil.vacuum);
        shared_ptr<OpElement<S>> zero_op =
            make_shared<OpElement<S>>(OpNames::Zero, SiteIndex(), hamil.vacuum);
        shared_ptr<OpExpr<S>> c_op[n_sites][2], d_op[n_sites][2];
        shared_ptr<OpExpr<S>> ccdd_op[n_sites][16];
        shared_ptr<OpExpr<S>> a_op[n_sites][n_sites][4];
        // here ad[i][j] = D_i * D_j
        shared_ptr<OpExpr<S>> ad_op[n_sites][n_sites][4];
        shared_ptr<OpExpr<S>> b_op[n_sites][n_sites][4];
        shared_ptr<OpExpr<S>> bd_op[n_sites][n_sites][4];
        // i <= j = k
        shared_ptr<OpExpr<S>> ccd_op[n_sites][n_sites][8];
        shared_ptr<OpExpr<S>> cdc_op[n_sites][n_sites][8];
        shared_ptr<OpExpr<S>> cdd_op[n_sites][n_sites][8];
        shared_ptr<OpExpr<S>> dcc_op[n_sites][n_sites][8];
        shared_ptr<OpExpr<S>> dcd_op[n_sites][n_sites][8];
        shared_ptr<OpExpr<S>> ddc_op[n_sites][n_sites][8];
        shared_ptr<OpExpr<S>> pdm2_op[n_sites][n_sites][n_sites][n_sites][16];
        const int sz[2] = {1, -1};
        const int sz_plus[4] = {2, 0, 0, -2}, sz_minus[4] = {0, -2, 2, 0};
        int sz_ccdd[16], sz_ccd[8], sz_cdc[8], sz_cdd[8], sz_dcc[8], sz_dcd[8],
            sz_ddc[8];
        for (uint8_t s = 0; s < 16; s++)
            sz_ccdd[s] = sz[SI(s)] + sz[SJ(s)] - sz[SK(s)] - sz[SL(s)];
        for (uint8_t s = 0; s < 8; s++) {
            sz_ccd[s] = +sz[SI(s)] + sz[SJ(s)] - sz[SK(s)];
            sz_cdc[s] = +sz[SI(s)] - sz[SJ(s)] + sz[SK(s)];
            sz_cdd[s] = +sz[SI(s)] - sz[SJ(s)] - sz[SK(s)];
            sz_dcc[s] = -sz[SI(s)] + sz[SJ(s)] + sz[SK(s)];
            sz_dcd[s] = -sz[SI(s)] + sz[SJ(s)] - sz[SK(s)];
            sz_ddc[s] = -sz[SI(s)] - sz[SJ(s)] + sz[SK(s)];
        }
        for (uint16_t m = 0; m < n_sites; m++) {
            for (uint8_t s = 0; s < 2; s++) {
                c_op[m][s] =
                    make_shared<OpElement<S>>(OpNames::C, SiteIndex({m}, {s}),
                                              S(1, sz[s], hamil.orb_sym[m]));
                d_op[m][s] =
                    make_shared<OpElement<S>>(OpNames::D, SiteIndex({m}, {s}),
                                              S(-1, -sz[s], hamil.orb_sym[m]));
            }
            for (uint8_t s = 0; s < 16; s++)
                ccdd_op[m][s] = make_shared<OpElement<S>>(
                    OpNames::CCDD,
                    SiteIndex({m, m, m, m}, {(uint8_t)SI(s), (uint8_t)SJ(s),
                                             (uint8_t)SK(s), (uint8_t)SL(s)}),
                    S(0, sz_ccdd[s], 0));
        }
        for (uint16_t i = 0; i < n_sites; i++)
            for (uint16_t j = 0; j < n_sites; j++)
                for (uint8_t s = 0; s < 4; s++) {
                    SiteIndex sidx({i, j}, {(uint8_t)SI(s), (uint8_t)SJ(s)});
                    SiteIndex sidx_ad({j, i}, {(uint8_t)SJ(s), (uint8_t)SI(s)});
                    a_op[i][j][s] = make_shared<OpElement<S>>(
                        OpNames::A, sidx,
                        S(2, sz_plus[s], hamil.orb_sym[i] ^ hamil.orb_sym[j]));
                    // note here ad is different from common def
                    // common def ad[i][j] = D_j * D_i
                    // here ad[i][j] = D_i * D_j
                    ad_op[i][j][s] = make_shared<OpElement<S>>(
                        OpNames::AD, sidx_ad,
                        S(-2, -sz_plus[s],
                          hamil.orb_sym[i] ^ hamil.orb_sym[j]));
                    b_op[i][j][s] = make_shared<OpElement<S>>(
                        OpNames::B, sidx,
                        S(0, sz_minus[s], hamil.orb_sym[i] ^ hamil.orb_sym[j]));
                    bd_op[i][j][s] = make_shared<OpElement<S>>(
                        OpNames::BD, sidx,
                        S(0, -sz_minus[s],
                          hamil.orb_sym[i] ^ hamil.orb_sym[j]));
                }
        for (uint16_t i = 0; i < n_sites; i++)
            for (uint16_t j = i; j < n_sites; j++)
                for (uint8_t s = 0; s < 8; s++) {
                    SiteIndex sidx({i, j, j}, {(uint8_t)SI(s), (uint8_t)SJ(s),
                                               (uint8_t)SK(s)});
                    ccd_op[i][j][s] = make_shared<OpElement<S>>(
                        OpNames::CCD, sidx, S(1, sz_ccd[s], hamil.orb_sym[i]));
                    cdc_op[i][j][s] = make_shared<OpElement<S>>(
                        OpNames::CDC, sidx, S(1, sz_cdc[s], hamil.orb_sym[i]));
                    cdd_op[i][j][s] = make_shared<OpElement<S>>(
                        OpNames::CDD, sidx, S(-1, sz_cdd[s], hamil.orb_sym[i]));
                    dcc_op[i][j][s] = make_shared<OpElement<S>>(
                        OpNames::DCC, sidx, S(1, sz_dcc[s], hamil.orb_sym[i]));
                    dcd_op[i][j][s] = make_shared<OpElement<S>>(
                        OpNames::DCD, sidx, S(-1, sz_dcd[s], hamil.orb_sym[i]));
                    ddc_op[i][j][s] = make_shared<OpElement<S>>(
                        OpNames::DDC, sidx, S(-1, sz_ddc[s], hamil.orb_sym[i]));
                }
        for (uint16_t i = 0; i < n_sites; i++)
            for (uint16_t j = 0; j < n_sites; j++)
                for (uint16_t k = 0; k < n_sites; k++)
                    for (uint16_t l = 0; l < n_sites; l++)
                        for (uint8_t s = 0; s < 16; s++) {
                            pdm2_op[i][j][k][l][s] = make_shared<OpElement<S>>(
                                OpNames::PDM2,
                                SiteIndex({i, j, k, l},
                                          {(uint8_t)SI(s), (uint8_t)SJ(s),
                                           (uint8_t)SK(s), (uint8_t)SL(s)}),
                                S(0, sz_ccdd[s],
                                  hamil.orb_sym[i] ^ hamil.orb_sym[j] ^
                                      hamil.orb_sym[k] ^ hamil.orb_sym[l]));
                        }
        MPO<S>::const_e = 0.0;
        MPO<S>::op = zero_op;
        MPO<S>::schemer = nullptr;
        MPO<S>::tf = make_shared<TensorFunctions<S>>(hamil.opf);
        MPO<S>::site_op_infos = hamil.site_op_infos;
        for (uint16_t m = 0; m < n_sites; m++) {
            // left operator names
            //   1 : identity
            //   1*16 : mmmm / ccdd
            //   2*8 : mmm / ccd cdd
            //   6*8*m : xmm / ccd cdc cdd dcc dcd ddc (x < m)
            //   4*4*(m+1) : xm / cc dd cd dc (x <= m)
            //   2*2*(m+1) : x / c d (x <= m)
            // right operator names
            //   1 : identity
            //   2*2*(n-m) : j / c d (j >= m)
            //   4*4*(n-m+1)*(n-m)/2 : jk / cc dd cd dc (j >= m, k >= j)
            //   2*8*(n-m) : jjj / ccd cdd (j >= m)
            //   1*16 : mmmm / ccdd (only last site)
            int lshape = m != n_sites - 1
                             ? 1 + 1 * 16 + 2 * 8 + 6 * 8 * m +
                                   4 * 4 * (m + 1) + 2 * 2 * (m + 1)
                             : 1;
            int rshape =
                m == 0
                    ? 1
                    : (m != n_sites - 1
                           ? 1 + 2 * 2 * (n_sites - m) +
                                 4 * 4 * (n_sites - m + 1) * (n_sites - m) / 2 +
                                 2 * 8 * (n_sites - m)
                           : 1 + 2 * 2 * (n_sites - m) +
                                 4 * 4 * (n_sites - m + 1) * (n_sites - m) / 2 +
                                 2 * 8 * (n_sites - m) + 1 * 16);
            // left operator names
            shared_ptr<SymbolicRowVector<S>> plop =
                make_shared<SymbolicRowVector<S>>(lshape);
            // 1 : identity
            (*plop)[0] = i_op;
            int p = 1;
            if (m != n_sites - 1) {
                // 1*16 : mmmm / ccdd
                for (uint8_t s = 0; s < 16; s++)
                    (*plop)[p + s] = ccdd_op[m][s];
                p += 16;
                // 2*8 : mmm / ccd cdd
                for (uint8_t s = 0; s < 8; s++)
                    (*plop)[p + s] = ccd_op[m][m][s];
                p += 8;
                for (uint8_t s = 0; s < 8; s++)
                    (*plop)[p + s] = cdd_op[m][m][s];
                p += 8;
                // 6*8*m : xmm / ccd cdc cdd dcc dcd ddc (x < m)
                for (uint8_t s = 0; s < 8; s++) {
                    for (uint16_t j = 0; j < m; j++)
                        (*plop)[p + j] = ccd_op[j][m][s];
                    p += m;
                    for (uint16_t j = 0; j < m; j++)
                        (*plop)[p + j] = cdc_op[j][m][s];
                    p += m;
                    for (uint16_t j = 0; j < m; j++)
                        (*plop)[p + j] = cdd_op[j][m][s];
                    p += m;
                    for (uint16_t j = 0; j < m; j++)
                        (*plop)[p + j] = dcc_op[j][m][s];
                    p += m;
                    for (uint16_t j = 0; j < m; j++)
                        (*plop)[p + j] = dcd_op[j][m][s];
                    p += m;
                    for (uint16_t j = 0; j < m; j++)
                        (*plop)[p + j] = ddc_op[j][m][s];
                    p += m;
                }
                // 4*4*(m+1) : xm / cc dd cd dc (x <= m)
                for (uint8_t s = 0; s < 4; s++) {
                    for (uint16_t j = 0; j <= m; j++)
                        (*plop)[p + j] = a_op[j][m][s];
                    p += m + 1;
                    for (uint16_t j = 0; j <= m; j++)
                        (*plop)[p + j] = ad_op[j][m][s];
                    p += m + 1;
                    for (uint16_t j = 0; j <= m; j++)
                        (*plop)[p + j] = b_op[j][m][s];
                    p += m + 1;
                    for (uint16_t j = 0; j <= m; j++)
                        (*plop)[p + j] = bd_op[j][m][s];
                    p += m + 1;
                }
                // 2*2*(m+1) : x / c d (x <= m)
                for (uint8_t s = 0; s < 2; s++) {
                    for (uint16_t j = 0; j <= m; j++)
                        (*plop)[p + j] = c_op[j][s];
                    p += m + 1;
                    for (uint16_t j = 0; j <= m; j++)
                        (*plop)[p + j] = d_op[j][s];
                    p += m + 1;
                }
            }
            assert(p == lshape);
            this->left_operator_names.push_back(plop);
            // right operator names
            shared_ptr<SymbolicColumnVector<S>> prop =
                make_shared<SymbolicColumnVector<S>>(rshape);
            // 1 : identity
            (*prop)[0] = i_op;
            p = 1;
            if (m != 0) {
                // 2*2*(n-m) : j / c d (j >= m)
                for (uint8_t s = 0; s < 2; s++) {
                    for (uint16_t j = m; j < n_sites; j++)
                        (*prop)[p + j - m] = c_op[j][s];
                    p += n_sites - m;
                    for (uint16_t j = m; j < n_sites; j++)
                        (*prop)[p + j - m] = d_op[j][s];
                    p += n_sites - m;
                }
                // 4*4*(n-m+1)*(n-m)/2 : jk / cc dd cd dc (j >= m, k >= j)
                for (uint8_t s = 0; s < 4; s++) {
                    for (uint16_t j = m; j < n_sites; j++) {
                        for (uint16_t k = j; k < n_sites; k++)
                            (*prop)[p + k - j] = a_op[j][k][s];
                        p += n_sites - j;
                    }
                    for (uint16_t j = m; j < n_sites; j++) {
                        for (uint16_t k = j; k < n_sites; k++)
                            (*prop)[p + k - j] = ad_op[j][k][s];
                        p += n_sites - j;
                    }
                    for (uint16_t j = m; j < n_sites; j++) {
                        for (uint16_t k = j; k < n_sites; k++)
                            (*prop)[p + k - j] = b_op[j][k][s];
                        p += n_sites - j;
                    }
                    for (uint16_t j = m; j < n_sites; j++) {
                        for (uint16_t k = j; k < n_sites; k++)
                            (*prop)[p + k - j] = bd_op[j][k][s];
                        p += n_sites - j;
                    }
                }
                // 2*8*(n-m) : jjj / ccd cdd (j >= m)
                for (uint8_t s = 0; s < 8; s++) {
                    for (uint16_t j = m; j < n_sites; j++)
                        (*prop)[p + j - m] = ccd_op[j][j][s];
                    p += n_sites - m;
                    for (uint16_t j = m; j < n_sites; j++)
                        (*prop)[p + j - m] = cdd_op[j][j][s];
                    p += n_sites - m;
                }
            }
            if (m == n_sites - 1) {
                // 1*16 : mmmm / ccdd (only last site)
                for (uint8_t s = 0; s < 16; s++)
                    (*prop)[p + s] = ccdd_op[m][s];
                p += 16;
            }
            assert(p == rshape);
            this->right_operator_names.push_back(prop);
            // middle operators
            //   1*16*1 : mmmm / ccdd
            //   4*16*(n-m-1) : mmmj(mmjm:mjmm:jmmm) / ccdd cddc (j > m)
            //   12*16*m*(n-m-1) :
            //      immj(imjm:ijmm:jmmi:jmim:jimm:mmij:mmji:mijm:mjim:mimj:mjmi)
            //      / ccdd cdcd cddc dccd dcdc ddcc (all) (i < m, j > m)
            //   6*16*(n-m-1) : mmjj(mjmj:mjjm:jjmm:jmjm:jmmj)
            //      / ccdd cdcd cddc dccd dcdc ddcc (all) (j > m)
            //   12*16*m*(n-m-1) :
            //      imjj(ijmj:ijjm:jjim:jijm:jimj:mijj:mjij:mjji:jjmi:jmji:jmij)
            //      / ccdd cdcd cddc dccd dcdc ddcc (all) (i < m, j > m)
            //   12*16*(n-m-2)*(n-m-1)/2 :
            //      mmjk(mmkj:mjmk:mkmj:mjkm:mkjm:jmmk:jmkm:jkmm:kmmj:kmjm:kjmm)
            //      / ccdd cdcd cddc dccd dcdc ddcc (all) (j > m, k > j)
            //   24*16*m*(n-m-2)*(n-m-1)/2 : imjk(:all)
            //      / ccdd cdcd cddc dccd dcdc ddcc (all) (i < m, j > m, k > j)
            //   4*16*(n-m-1) : mjjj(jmjj:jjmj:jjjm) / ccdd cddc (j > m)
            //   1*16*1 : jjjj / ccdd (j > m) (last site only)
            if (m != n_sites - 1) {
                int mshape = 16 * (1 * 1 + 4 * (n_sites - m - 1) +
                                   12 * m * (n_sites - m - 1) +
                                   6 * (2 * m + 1) * (n_sites - m - 1) +
                                   12 * (2 * m + 1) * (n_sites - m - 2) *
                                       (n_sites - m - 1) / 2 +
                                   4 * (n_sites - m - 1));
                if (m == n_sites - 2)
                    mshape += 16 * 1;
                shared_ptr<SymbolicColumnVector<S>> pmop =
                    make_shared<SymbolicColumnVector<S>>(mshape);
                shared_ptr<SymbolicColumnVector<S>> pmexpr =
                    make_shared<SymbolicColumnVector<S>>(mshape);
                p = 0;
                for (uint8_t s = 0; s < 16; s++) {
                    // 1*16*1 : mmmm / ccdd
                    (*pmop)[p] = pdm2_op[m][m][m][m][s];
                    (*pmexpr)[p] = ccdd_op[m][s] * i_op;
                    p++;
                    // 4*16*(n-m-1) : mmmj(-mmjm:+mjmm:-jmmm) / ccdd cddc (j >
                    // m)
                    for (uint16_t j = m + 1; j < n_sites; j++) {
                        (*pmop)[p] = pdm2_op[m][m][m][j][s];
                        (*pmexpr)[p] = ccd_op[m][m][PIJK(SI(s), SJ(s), SK(s))] *
                                       d_op[j][SL(s)];
                        p++;
                        (*pmop)[p] = pdm2_op[m][m][j][m][s];
                        (*pmexpr)[p] =
                            -1.0 * (ccd_op[m][m][PIJK(SI(s), SJ(s), SL(s))] *
                                    d_op[j][SK(s)]);
                        p++;
                        (*pmop)[p] = pdm2_op[m][j][m][m][s];
                        (*pmexpr)[p] = cdd_op[m][m][PIJK(SI(s), SK(s), SL(s))] *
                                       c_op[j][SJ(s)];
                        p++;
                        (*pmop)[p] = pdm2_op[j][m][m][m][s];
                        (*pmexpr)[p] =
                            -1.0 * (cdd_op[m][m][PIJK(SJ(s), SK(s), SL(s))] *
                                    c_op[j][PI(SI(s))]);
                        p++;
                    }
                    //   12*16*m*(n-m-1) :
                    //      immj(-imjm:+ijmm:-jmmi:+jmim:-jimm:+mmij:-mmji:+mijm:-mjim:-mimj:+mjmi)
                    //      / ccdd cdcd cddc dccd dcdc ddcc (all) (i < m, j > m)
                    for (uint16_t i = 0; i < m; i++)
                        for (uint16_t j = m + 1; j < n_sites; j++) {
                            (*pmop)[p] = pdm2_op[i][m][m][j][s];
                            (*pmexpr)[p] =
                                ccd_op[i][m][PIJK(SI(s), SJ(s), SK(s))] *
                                d_op[j][SL(s)];
                            p++;
                            (*pmop)[p] = pdm2_op[i][m][j][m][s];
                            (*pmexpr)[p] =
                                -1.0 *
                                (ccd_op[i][m][PIJK(SI(s), SJ(s), SL(s))] *
                                 d_op[j][SK(s)]);
                            p++;
                            (*pmop)[p] = pdm2_op[i][j][m][m][s];
                            (*pmexpr)[p] =
                                cdd_op[i][m][PIJK(SI(s), SK(s), SL(s))] *
                                c_op[j][SJ(s)];
                            p++;
                            (*pmop)[p] = pdm2_op[j][m][m][i][s];
                            (*pmexpr)[p] =
                                -1.0 *
                                (dcd_op[i][m][PIJK(SL(s), SJ(s), SK(s))] *
                                 c_op[j][SI(s)]);
                            p++;
                            (*pmop)[p] = pdm2_op[j][m][i][m][s];
                            (*pmexpr)[p] =
                                dcd_op[i][m][PIJK(SK(s), SJ(s), SL(s))] *
                                c_op[j][SI(s)];
                            p++;
                            (*pmop)[p] = pdm2_op[j][i][m][m][s];
                            (*pmexpr)[p] =
                                -1.0 *
                                (cdd_op[i][m][PIJK(SJ(s), SK(s), SL(s))] *
                                 c_op[j][SI(s)]);
                            p++;
                            (*pmop)[p] = pdm2_op[m][m][i][j][s];
                            (*pmexpr)[p] =
                                dcc_op[i][m][PIJK(SK(s), SI(s), SJ(s))] *
                                d_op[j][SL(s)];
                            p++;
                            (*pmop)[p] = pdm2_op[m][m][j][i][s];
                            (*pmexpr)[p] =
                                -1.0 *
                                (dcc_op[i][m][PIJK(SL(s), SI(s), SJ(s))] *
                                 d_op[j][SK(s)]);
                            p++;
                            (*pmop)[p] = pdm2_op[m][i][j][m][s];
                            (*pmexpr)[p] =
                                ccd_op[i][m][PIJK(SJ(s), SI(s), SL(s))] *
                                d_op[j][SK(s)];
                            p++;
                            (*pmop)[p] = pdm2_op[m][j][i][m][s];
                            (*pmexpr)[p] =
                                -1.0 *
                                (dcd_op[i][m][PIJK(SK(s), SI(s), SL(s))] *
                                 c_op[j][SJ(s)]);
                            p++;
                            (*pmop)[p] = pdm2_op[m][i][m][j][s];
                            (*pmexpr)[p] =
                                -1.0 *
                                (ccd_op[i][m][PIJK(SJ(s), SI(s), SK(s))] *
                                 d_op[j][SL(s)]);
                            p++;
                            (*pmop)[p] = pdm2_op[m][j][m][i][s];
                            (*pmexpr)[p] =
                                dcd_op[i][m][PIJK(SL(s), SI(s), SK(s))] *
                                c_op[j][SJ(s)];
                            p++;
                        }
                    //   6*16*(n-m-1) : mmjj(-mjmj:+mjjm:+jjmm:-jmjm:+jmmj)
                    //      / ccdd cdcd cddc dccd dcdc ddcc (all) (j > m)
                    for (uint16_t j = m + 1; j < n_sites; j++) {
                        (*pmop)[p] = pdm2_op[m][m][j][j][s];
                        (*pmexpr)[p] = a_op[m][m][PIJ(SI(s), SJ(s))] *
                                       ad_op[j][j][PIJ(SK(s), SL(s))];
                        p++;
                        (*pmop)[p] = pdm2_op[m][j][m][j][s];
                        (*pmexpr)[p] = -1.0 * (b_op[m][m][PIJ(SI(s), SK(s))] *
                                               b_op[j][j][PIJ(SJ(s), SL(s))]);
                        p++;
                        (*pmop)[p] = pdm2_op[m][j][j][m][s];
                        (*pmexpr)[p] = b_op[m][m][PIJ(SI(s), SL(s))] *
                                       b_op[j][j][PIJ(SJ(s), SK(s))];
                        p++;
                        (*pmop)[p] = pdm2_op[j][j][m][m][s];
                        (*pmexpr)[p] = ad_op[m][m][PIJ(SK(s), SL(s))] *
                                       a_op[j][j][PIJ(SI(s), SJ(s))];
                        p++;
                        (*pmop)[p] = pdm2_op[j][m][j][m][s];
                        (*pmexpr)[p] = -1.0 * (b_op[m][m][PIJ(SJ(s), SL(s))] *
                                               b_op[j][j][PIJ(SI(s), SK(s))]);
                        p++;
                        (*pmop)[p] = pdm2_op[j][m][m][j][s];
                        (*pmexpr)[p] = b_op[m][m][PIJ(SJ(s), SK(s))] *
                                       b_op[j][j][PIJ(SI(s), SL(s))];
                        p++;
                    }
                    //   12*16*m*(n-m-1) :
                    //      imjj(-ijmj:+ijjm:+jjim:-jijm:+jimj:-mijj:+mjij:-mjji:-jjmi:+jmji:-jmij)
                    //      / ccdd cdcd cddc dccd dcdc ddcc (all) (i < m, j > m)
                    for (uint16_t i = 0; i < m; i++)
                        for (uint16_t j = m + 1; j < n_sites; j++) {
                            (*pmop)[p] = pdm2_op[i][m][j][j][s];
                            (*pmexpr)[p] = a_op[i][m][PIJ(SI(s), SJ(s))] *
                                           ad_op[j][j][PIJ(SK(s), SL(s))];
                            p++;
                            (*pmop)[p] = pdm2_op[i][j][m][j][s];
                            (*pmexpr)[p] =
                                -1.0 * (b_op[i][m][PIJ(SI(s), SK(s))] *
                                        b_op[j][j][PIJ(SJ(s), SL(s))]);
                            p++;
                            (*pmop)[p] = pdm2_op[i][j][j][m][s];
                            (*pmexpr)[p] = b_op[i][m][PIJ(SI(s), SL(s))] *
                                           b_op[j][j][PIJ(SJ(s), SK(s))];
                            p++;
                            (*pmop)[p] = pdm2_op[j][j][i][m][s];
                            (*pmexpr)[p] = ad_op[i][m][PIJ(SK(s), SL(s))] *
                                           a_op[j][j][PIJ(SI(s), SJ(s))];
                            p++;
                            (*pmop)[p] = pdm2_op[j][i][j][m][s];
                            (*pmexpr)[p] =
                                -1.0 * (b_op[i][m][PIJ(SJ(s), SL(s))] *
                                        b_op[j][j][PIJ(SI(s), SK(s))]);
                            p++;
                            (*pmop)[p] = pdm2_op[j][i][m][j][s];
                            (*pmexpr)[p] = b_op[i][m][PIJ(SJ(s), SK(s))] *
                                           b_op[j][j][PIJ(SI(s), SL(s))];
                            p++;
                            (*pmop)[p] = pdm2_op[m][i][j][j][s];
                            (*pmexpr)[p] =
                                -1.0 * (a_op[i][m][PIJ(SJ(s), SI(s))] *
                                        ad_op[j][j][PIJ(SK(s), SL(s))]);
                            p++;
                            (*pmop)[p] = pdm2_op[m][j][i][j][s];
                            (*pmexpr)[p] = bd_op[i][m][PIJ(SK(s), SI(s))] *
                                           b_op[j][j][PIJ(SJ(s), SL(s))];
                            p++;
                            (*pmop)[p] = pdm2_op[m][j][j][i][s];
                            (*pmexpr)[p] =
                                -1.0 * (bd_op[i][m][PIJ(SL(s), SI(s))] *
                                        b_op[j][j][PIJ(SJ(s), SK(s))]);
                            p++;
                            (*pmop)[p] = pdm2_op[j][j][m][i][s];
                            (*pmexpr)[p] =
                                -1.0 * (ad_op[i][m][PIJ(SL(s), SK(s))] *
                                        a_op[j][j][PIJ(SI(s), SJ(s))]);
                            p++;
                            (*pmop)[p] = pdm2_op[j][m][j][i][s];
                            (*pmexpr)[p] = bd_op[i][m][PIJ(SL(s), SJ(s))] *
                                           b_op[j][j][PIJ(SI(s), SK(s))];
                            p++;
                            (*pmop)[p] = pdm2_op[j][m][i][j][s];
                            (*pmexpr)[p] =
                                -1.0 * (bd_op[i][m][PIJ(SK(s), SJ(s))] *
                                        b_op[j][j][PIJ(SI(s), SL(s))]);
                            p++;
                        }
                    //   12*16*(n-m-2)*(n-m-1)/2 :
                    //      mmjk(-mmkj:-mjmk:+mkmj:+mjkm:-mkjm:+jmmk:-jmkm:+jkmm:-kmmj:+kmjm:-kjmm)
                    //      / ccdd cdcd cddc dccd dcdc ddcc (all) (j > m, k > j)
                    for (uint16_t j = m + 1; j < n_sites; j++)
                        for (uint16_t k = j + 1; k < n_sites; k++) {
                            (*pmop)[p] = pdm2_op[m][m][j][k][s];
                            (*pmexpr)[p] = a_op[m][m][PIJ(SI(s), SJ(s))] *
                                           ad_op[j][k][PIJ(SK(s), SL(s))];
                            p++;
                            (*pmop)[p] = pdm2_op[m][m][k][j][s];
                            (*pmexpr)[p] =
                                -1.0 * (a_op[m][m][PIJ(SI(s), SJ(s))] *
                                        ad_op[j][k][PIJ(SL(s), SK(s))]);
                            p++;
                            (*pmop)[p] = pdm2_op[m][j][m][k][s];
                            (*pmexpr)[p] =
                                -1.0 * (b_op[m][m][PIJ(SI(s), SK(s))] *
                                        b_op[j][k][PIJ(SJ(s), SL(s))]);
                            p++;
                            (*pmop)[p] = pdm2_op[m][k][m][j][s];
                            (*pmexpr)[p] = b_op[m][m][PIJ(SI(s), SK(s))] *
                                           bd_op[j][k][PIJ(SL(s), SJ(s))];
                            p++;
                            (*pmop)[p] = pdm2_op[m][j][k][m][s];
                            (*pmexpr)[p] = b_op[m][m][PIJ(SI(s), SL(s))] *
                                           b_op[j][k][PIJ(SJ(s), SK(s))];
                            p++;
                            (*pmop)[p] = pdm2_op[m][k][j][m][s];
                            (*pmexpr)[p] =
                                -1.0 * (b_op[m][m][PIJ(SI(s), SL(s))] *
                                        bd_op[j][k][PIJ(SK(s), SJ(s))]);
                            p++;
                            (*pmop)[p] = pdm2_op[j][m][m][k][s];
                            (*pmexpr)[p] = b_op[m][m][PIJ(SJ(s), SK(s))] *
                                           b_op[j][k][PIJ(SI(s), SL(s))];
                            p++;
                            (*pmop)[p] = pdm2_op[j][m][k][m][s];
                            (*pmexpr)[p] =
                                -1.0 * (b_op[m][m][PIJ(SJ(s), SL(s))] *
                                        b_op[j][k][PIJ(SI(s), SK(s))]);
                            p++;
                            (*pmop)[p] = pdm2_op[j][k][m][m][s];
                            (*pmexpr)[p] = ad_op[m][m][PIJ(SK(s), SL(s))] *
                                           a_op[j][k][PIJ(SI(s), SJ(s))];
                            p++;
                            (*pmop)[p] = pdm2_op[k][m][m][j][s];
                            (*pmexpr)[p] =
                                -1.0 * (b_op[m][m][PIJ(SJ(s), SK(s))] *
                                        bd_op[j][k][PIJ(SL(s), SI(s))]);
                            p++;
                            (*pmop)[p] = pdm2_op[k][m][j][m][s];
                            (*pmexpr)[p] = b_op[m][m][PIJ(SJ(s), SL(s))] *
                                           bd_op[j][k][PIJ(SK(s), SI(s))];
                            p++;
                            (*pmop)[p] = pdm2_op[k][j][m][m][s];
                            (*pmexpr)[p] =
                                -1.0 * (ad_op[m][m][PIJ(SK(s), SL(s))] *
                                        a_op[j][k][PIJ(SJ(s), SI(s))]);
                            p++;
                        }
                    // 24*16*m*(n-m-2)*(n-m-1)/2 :
                    //    (+imjk:-imkj:-ijmk:+ijkm:+ikmj:-ikjm)
                    //    (-mijk:+mikj:+mjik:-mjki:-mkij:+mkji)
                    //    (+jimk:-jikm:-jmik:+jmki:+jkim:-jkmi)
                    //    (-kimj:+kijm:+kmij:-kmji:-kjim:+kjmi)
                    //    / ccdd cdcd cddc dccd dcdc ddcc (all) (i < m, j > m, k
                    //    > j)
                    for (uint16_t i = 0; i < m; i++)
                        for (uint16_t j = m + 1; j < n_sites; j++)
                            for (uint16_t k = j + 1; k < n_sites; k++) {
                                // (+imjk:-imkj:-ijmk:+ijkm:+ikmj:-ikjm)
                                (*pmop)[p] = pdm2_op[i][m][j][k][s];
                                (*pmexpr)[p] = a_op[i][m][PIJ(SI(s), SJ(s))] *
                                               ad_op[j][k][PIJ(SK(s), SL(s))];
                                p++;
                                (*pmop)[p] = pdm2_op[i][m][k][j][s];
                                (*pmexpr)[p] =
                                    -1.0 * (a_op[i][m][PIJ(SI(s), SJ(s))] *
                                            ad_op[j][k][PIJ(SL(s), SK(s))]);
                                p++;
                                (*pmop)[p] = pdm2_op[i][j][m][k][s];
                                (*pmexpr)[p] =
                                    -1.0 * (b_op[i][m][PIJ(SI(s), SK(s))] *
                                            b_op[j][k][PIJ(SJ(s), SL(s))]);
                                p++;
                                (*pmop)[p] = pdm2_op[i][j][k][m][s];
                                (*pmexpr)[p] = b_op[i][m][PIJ(SI(s), SL(s))] *
                                               b_op[j][k][PIJ(SJ(s), SK(s))];
                                p++;
                                (*pmop)[p] = pdm2_op[i][k][m][j][s];
                                (*pmexpr)[p] = b_op[i][m][PIJ(SI(s), SK(s))] *
                                               bd_op[j][k][PIJ(SL(s), SJ(s))];
                                p++;
                                (*pmop)[p] = pdm2_op[i][k][j][m][s];
                                (*pmexpr)[p] =
                                    -1.0 * (b_op[i][m][PIJ(SI(s), SL(s))] *
                                            bd_op[j][k][PIJ(SK(s), SJ(s))]);
                                p++;
                                // (-mijk:+mikj:+mjik:-mjki:-mkij:+mkji)
                                (*pmop)[p] = pdm2_op[m][i][j][k][s];
                                (*pmexpr)[p] =
                                    -1.0 * (a_op[i][m][PIJ(SJ(s), SI(s))] *
                                            ad_op[j][k][PIJ(SK(s), SL(s))]);
                                p++;
                                (*pmop)[p] = pdm2_op[m][i][k][j][s];
                                (*pmexpr)[p] = a_op[i][m][PIJ(SJ(s), SI(s))] *
                                               ad_op[j][k][PIJ(SL(s), SK(s))];
                                p++;
                                (*pmop)[p] = pdm2_op[m][j][i][k][s];
                                (*pmexpr)[p] = bd_op[i][m][PIJ(SK(s), SI(s))] *
                                               b_op[j][k][PIJ(SJ(s), SL(s))];
                                p++;
                                (*pmop)[p] = pdm2_op[m][j][k][i][s];
                                (*pmexpr)[p] =
                                    -1.0 * (bd_op[i][m][PIJ(SL(s), SI(s))] *
                                            b_op[j][k][PIJ(SJ(s), SK(s))]);
                                p++;
                                (*pmop)[p] = pdm2_op[m][k][i][j][s];
                                (*pmexpr)[p] =
                                    -1.0 * (bd_op[i][m][PIJ(SK(s), SI(s))] *
                                            bd_op[j][k][PIJ(SL(s), SJ(s))]);
                                p++;
                                (*pmop)[p] = pdm2_op[m][k][j][i][s];
                                (*pmexpr)[p] = bd_op[i][m][PIJ(SL(s), SI(s))] *
                                               bd_op[j][k][PIJ(SK(s), SJ(s))];
                                p++;
                                // (+jimk:-jikm:-jmik:+jmki:+jkim:-jkmi)
                                (*pmop)[p] = pdm2_op[j][i][m][k][s];
                                (*pmexpr)[p] = b_op[i][m][PIJ(SJ(s), SK(s))] *
                                               b_op[j][k][PIJ(SI(s), SL(s))];
                                p++;
                                (*pmop)[p] = pdm2_op[j][i][k][m][s];
                                (*pmexpr)[p] =
                                    -1.0 * (b_op[i][m][PIJ(SJ(s), SL(s))] *
                                            b_op[j][k][PIJ(SI(s), SK(s))]);
                                p++;
                                (*pmop)[p] = pdm2_op[j][m][i][k][s];
                                (*pmexpr)[p] =
                                    -1.0 * (bd_op[i][m][PIJ(SK(s), SJ(s))] *
                                            b_op[j][k][PIJ(SI(s), SL(s))]);
                                p++;
                                (*pmop)[p] = pdm2_op[j][m][k][i][s];
                                (*pmexpr)[p] = bd_op[i][m][PIJ(SL(s), SJ(s))] *
                                               b_op[j][k][PIJ(SI(s), SK(s))];
                                p++;
                                (*pmop)[p] = pdm2_op[j][k][i][m][s];
                                (*pmexpr)[p] = ad_op[i][m][PIJ(SK(s), SL(s))] *
                                               a_op[j][k][PIJ(SI(s), SJ(s))];
                                p++;
                                (*pmop)[p] = pdm2_op[j][k][m][i][s];
                                (*pmexpr)[p] =
                                    -1.0 * (ad_op[i][m][PIJ(SL(s), SK(s))] *
                                            a_op[j][k][PIJ(SI(s), SJ(s))]);
                                p++;
                                // (-kimj:+kijm:+kmij:-kmji:-kjim:+kjmi)
                                (*pmop)[p] = pdm2_op[k][i][m][j][s];
                                (*pmexpr)[p] =
                                    -1.0 * (b_op[i][m][PIJ(SJ(s), SK(s))] *
                                            bd_op[j][k][PIJ(SL(s), SI(s))]);
                                p++;
                                (*pmop)[p] = pdm2_op[k][i][j][m][s];
                                (*pmexpr)[p] = b_op[i][m][PIJ(SJ(s), SL(s))] *
                                               bd_op[j][k][PIJ(SK(s), SI(s))];
                                p++;
                                (*pmop)[p] = pdm2_op[k][m][i][j][s];
                                (*pmexpr)[p] = bd_op[i][m][PIJ(SK(s), SJ(s))] *
                                               bd_op[j][k][PIJ(SL(s), SI(s))];
                                p++;
                                (*pmop)[p] = pdm2_op[k][m][j][i][s];
                                (*pmexpr)[p] =
                                    -1.0 * (bd_op[i][m][PIJ(SL(s), SJ(s))] *
                                            bd_op[j][k][PIJ(SK(s), SI(s))]);
                                p++;
                                (*pmop)[p] = pdm2_op[k][j][i][m][s];
                                (*pmexpr)[p] =
                                    -1.0 * (ad_op[i][m][PIJ(SK(s), SL(s))] *
                                            a_op[j][k][PIJ(SJ(s), SI(s))]);
                                p++;
                                (*pmop)[p] = pdm2_op[k][j][m][i][s];
                                (*pmexpr)[p] = ad_op[i][m][PIJ(SL(s), SK(s))] *
                                               a_op[j][k][PIJ(SJ(s), SI(s))];
                                p++;
                            }
                    // 4*16*(n-m-1) : mjjj(-jmjj:+jjmj:-jjjm) / ccdd cddc (j >
                    // m)
                    for (uint16_t j = m + 1; j < n_sites; j++) {
                        (*pmop)[p] = pdm2_op[m][j][j][j][s];
                        (*pmexpr)[p] = c_op[m][SI(s)] *
                                       cdd_op[j][j][PIJK(SJ(s), SK(s), SL(s))];
                        p++;
                        (*pmop)[p] = pdm2_op[j][m][j][j][s];
                        (*pmexpr)[p] =
                            -1.0 * (c_op[m][SJ(s)] *
                                    cdd_op[j][j][PIJK(SI(s), SK(s), SL(s))]);
                        p++;
                        (*pmop)[p] = pdm2_op[j][j][m][j][s];
                        (*pmexpr)[p] = d_op[m][SK(s)] *
                                       ccd_op[j][j][PIJK(SI(s), SJ(s), SL(s))];
                        p++;
                        (*pmop)[p] = pdm2_op[j][j][j][m][s];
                        (*pmexpr)[p] =
                            -1.0 * (d_op[m][SL(s)] *
                                    ccd_op[j][j][PIJK(SI(s), SJ(s), SK(s))]);
                        p++;
                    }
                }
                // 1*16*1 : jjjj / ccdd (j > m) (last site only)
                if (m == n_sites - 2)
                    for (uint8_t s = 0; s < 16; s++) {
                        (*pmop)[p] = pdm2_op[m + 1][m + 1][m + 1][m + 1][s];
                        (*pmexpr)[p] = i_op * ccdd_op[m + 1][s];
                        p++;
                    }
                assert(p == mshape);
                this->middle_operator_names.push_back(pmop);
                this->middle_operator_exprs.push_back(pmexpr);
            }
            // site tensors
            shared_ptr<OperatorTensor<S>> opt =
                make_shared<OperatorTensor<S>>();
            // left operator names
            //   1 : identity
            //   1*16 : mmmm / ccdd
            //   2*8 : mmm / ccd cdd
            //   6*8*m : xmm / ccd cdc cdd dcc dcd ddc (x < m)
            //   4*4*(m+1) : xm / cc dd cd dc (x <= m)
            //   2*2*(m+1) : x / c d (x <= m)
            int llshape =
                1 + 1 * 16 + 2 * 8 + 6 * 8 * (m - 1) + 4 * 4 * m + 2 * 2 * m;
            int lrshape = m != n_sites - 1
                              ? 1 + 1 * 16 + 2 * 8 + 6 * 8 * m +
                                    4 * 4 * (m + 1) + 2 * 2 * (m + 1)
                              : 1;
            shared_ptr<Symbolic<S>> plmat = nullptr, prmat = nullptr;
            if (m == 0)
                plmat = make_shared<SymbolicRowVector<S>>(lrshape);
            else if (m == n_sites - 1)
                plmat = make_shared<SymbolicColumnVector<S>>(llshape);
            else
                plmat = make_shared<SymbolicMatrix<S>>(llshape, lrshape);
            (*plmat)[{0, 0}] = i_op;
            p = 1;
            if (m != n_sites - 1) {
                int pi = 0;
                int pc[2] = {1 + 1 * 16 + 2 * 8 + 6 * 8 * (m - 1) + 4 * 4 * m,
                             1 + 1 * 16 + 2 * 8 + 6 * 8 * (m - 1) + 4 * 4 * m +
                                 2 * m};
                int pd[2] = {
                    1 + 1 * 16 + 2 * 8 + 6 * 8 * (m - 1) + 4 * 4 * m + m,
                    1 + 1 * 16 + 2 * 8 + 6 * 8 * (m - 1) + 4 * 4 * m + 3 * m};
                // 1*16 : mmmm / ccdd
                for (uint8_t s = 0; s < 16; s++)
                    (*plmat)[{pi, p + s}] = ccdd_op[m][s];
                p += 16;
                // 1*16 : mmmm / ccdd
                for (uint8_t s = 0; s < 8; s++)
                    (*plmat)[{pi, p + s}] = ccd_op[m][m][s];
                p += 8;
                for (uint8_t s = 0; s < 8; s++)
                    (*plmat)[{pi, p + s}] = cdd_op[m][m][s];
                p += 8;
                // 6*8*m : xmm / ccd cdc cdd dcc dcd ddc (x < m)
                for (uint8_t s = 0; s < 8; s++) {
                    for (uint16_t j = 0; j < m; j++)
                        (*plmat)[{pc[SI(s)] + j, p + j}] =
                            b_op[m][m][PIJ(SJ(s), SK(s))];
                    p += m;
                    for (uint16_t j = 0; j < m; j++)
                        (*plmat)[{pc[SI(s)] + j, p + j}] =
                            bd_op[m][m][PIJ(SJ(s), SK(s))];
                    p += m;
                    for (uint16_t j = 0; j < m; j++)
                        (*plmat)[{pc[SI(s)] + j, p + j}] =
                            ad_op[m][m][PIJ(SJ(s), SK(s))];
                    p += m;
                    for (uint16_t j = 0; j < m; j++)
                        (*plmat)[{pd[SI(s)] + j, p + j}] =
                            a_op[m][m][PIJ(SJ(s), SK(s))];
                    p += m;
                    for (uint16_t j = 0; j < m; j++)
                        (*plmat)[{pd[SI(s)] + j, p + j}] =
                            b_op[m][m][PIJ(SJ(s), SK(s))];
                    p += m;
                    for (uint16_t j = 0; j < m; j++)
                        (*plmat)[{pd[SI(s)] + j, p + j}] =
                            bd_op[m][m][PIJ(SJ(s), SK(s))];
                    p += m;
                }
                // 4*4*(m+1) : xm / cc dd cd dc (x <= m)
                for (uint8_t s = 0; s < 4; s++) {
                    for (uint16_t j = 0; j < m; j++)
                        (*plmat)[{pc[SI(s)] + j, p + j}] = c_op[m][SJ(s)];
                    (*plmat)[{pi, p + m}] = a_op[m][m][s];
                    p += m + 1;
                    for (uint16_t j = 0; j < m; j++)
                        (*plmat)[{pd[SI(s)] + j, p + j}] = d_op[m][SJ(s)];
                    (*plmat)[{pi, p + m}] = ad_op[m][m][s];
                    p += m + 1;
                    for (uint16_t j = 0; j < m; j++)
                        (*plmat)[{pc[SI(s)] + j, p + j}] = d_op[m][SJ(s)];
                    (*plmat)[{pi, p + m}] = b_op[m][m][s];
                    p += m + 1;
                    for (uint16_t j = 0; j < m; j++)
                        (*plmat)[{pd[SI(s)] + j, p + j}] = c_op[m][SJ(s)];
                    (*plmat)[{pi, p + m}] = bd_op[m][m][s];
                    p += m + 1;
                }
                // 2*2*(m+1) : x / c d (x <= m)
                for (uint8_t s = 0; s < 2; s++) {
                    for (uint16_t j = 0; j < m; j++)
                        (*plmat)[{pc[s] + j, p + j}] = i_op;
                    (*plmat)[{pi, p + m}] = c_op[m][s];
                    p += m + 1;
                    for (uint16_t j = 0; j < m; j++)
                        (*plmat)[{pd[s] + j, p + j}] = i_op;
                    (*plmat)[{pi, p + m}] = d_op[m][s];
                    p += m + 1;
                }
            }
            assert(p == lrshape);
            // right operator names
            //   1 : identity
            //   2*2*(n-m) : j / c d (j >= m)
            //   4*4*(n-m+1)*(n-m)/2 : jk / cc dd cd dc (j >= m, k >= j)
            //   2*8*(n-m) : jjj / ccd cdd (j >= m)
            //   1*16 : mmmm / ccdd (only last site)
            int rlshape =
                m == 0
                    ? 1
                    : (m != n_sites - 1
                           ? 1 + 2 * 2 * (n_sites - m) +
                                 4 * 4 * (n_sites - m + 1) * (n_sites - m) / 2 +
                                 2 * 8 * (n_sites - m)
                           : 1 + 2 * 2 * (n_sites - m) +
                                 4 * 4 * (n_sites - m + 1) * (n_sites - m) / 2 +
                                 2 * 8 * (n_sites - m) + 1 * 16);
            int rrshape = m != n_sites - 2
                              ? 1 + 2 * 2 * (n_sites - m - 1) +
                                    4 * 4 * (n_sites - m - 1 + 1) *
                                        (n_sites - m - 1) / 2 +
                                    2 * 8 * (n_sites - m - 1)
                              : 1 + 2 * 2 * (n_sites - m - 1) +
                                    4 * 4 * (n_sites - m - 1 + 1) *
                                        (n_sites - m - 1) / 2 +
                                    2 * 8 * (n_sites - m - 1) + 1 * 16;
            if (m == 0)
                prmat = make_shared<SymbolicRowVector<S>>(rrshape);
            else if (m == n_sites - 1)
                prmat = make_shared<SymbolicColumnVector<S>>(rlshape);
            else
                prmat = make_shared<SymbolicMatrix<S>>(rlshape, rrshape);
            (*prmat)[{0, 0}] = i_op;
            p = 1;
            if (m != 0) {
                // 2*2*(n-m) : j / c d (j >= m)
                int pi = 0;
                int pc[2] = {1 - (m + 1), 1 + 2 * (n_sites - m - 1) - (m + 1)},
                    pd[2] = {1 + (n_sites - m - 1) - (m + 1),
                             1 + 3 * (n_sites - m - 1) - (m + 1)};
                for (uint8_t s = 0; s < 2; s++) {
                    (*prmat)[{p, pi}] = c_op[m][s];
                    for (uint16_t j = m + 1; j < n_sites; j++)
                        (*prmat)[{p + j - m, pc[s] + j}] = i_op;
                    p += n_sites - m;
                    (*prmat)[{p, pi}] = d_op[m][s];
                    for (uint16_t j = m + 1; j < n_sites; j++)
                        (*prmat)[{p + j - m, pd[s] + j}] = i_op;
                    p += n_sites - m;
                }
                // 4*4*(n-m+1)*(n-m)/2 : jk / cc dd cd dc (j >= m, k >= j)
                int pp = 1 + 4 * (n_sites - m - 1);
                for (uint8_t s = 0; s < 4; s++) {
                    (*prmat)[{p, pi}] = a_op[m][m][s];
                    for (uint16_t k = m + 1; k < n_sites; k++)
                        (*prmat)[{p + k - m, pc[SJ(s)] + k}] = c_op[m][SI(s)];
                    p += n_sites - m;
                    for (uint16_t j = m + 1; j < n_sites; j++) {
                        for (uint16_t k = j; k < n_sites; k++)
                            (*prmat)[{p + k - j, pp + k - j}] = i_op;
                        p += n_sites - j, pp += n_sites - j;
                    }
                    (*prmat)[{p, pi}] = ad_op[m][m][s];
                    for (uint16_t k = m + 1; k < n_sites; k++)
                        (*prmat)[{p + k - m, pd[SJ(s)] + k}] = d_op[m][SI(s)];
                    p += n_sites - m;
                    for (uint16_t j = m + 1; j < n_sites; j++) {
                        for (uint16_t k = j; k < n_sites; k++)
                            (*prmat)[{p + k - j, pp + k - j}] = i_op;
                        p += n_sites - j, pp += n_sites - j;
                    }
                    (*prmat)[{p, pi}] = b_op[m][m][s];
                    for (uint16_t k = m + 1; k < n_sites; k++)
                        (*prmat)[{p + k - m, pd[SJ(s)] + k}] = c_op[m][SI(s)];
                    p += n_sites - m;
                    for (uint16_t j = m + 1; j < n_sites; j++) {
                        for (uint16_t k = j; k < n_sites; k++)
                            (*prmat)[{p + k - j, pp + k - j}] = i_op;
                        p += n_sites - j, pp += n_sites - j;
                    }
                    (*prmat)[{p, pi}] = bd_op[m][m][s];
                    for (uint16_t k = m + 1; k < n_sites; k++)
                        (*prmat)[{p + k - m, pc[SJ(s)] + k}] = d_op[m][SI(s)];
                    p += n_sites - m;
                    for (uint16_t j = m + 1; j < n_sites; j++) {
                        for (uint16_t k = j; k < n_sites; k++)
                            (*prmat)[{p + k - j, pp + k - j}] = i_op;
                        p += n_sites - j, pp += n_sites - j;
                    }
                }
                assert(pp == 1 + 4 * (n_sites - m - 1) +
                                 4 * 4 * (n_sites - m - 1 + 1) *
                                     (n_sites - m - 1) / 2);
                // 2*8*(n-m) : jjj / ccd cdd (j >= m)
                for (uint8_t s = 0; s < 8; s++) {
                    (*prmat)[{p, pi}] = ccd_op[m][m][s];
                    for (uint16_t j = m + 1; j < n_sites; j++)
                        (*prmat)[{p + j - m, pp + j - m - 1}] = i_op;
                    p += n_sites - m, pp += n_sites - m - 1;
                    (*prmat)[{p, pi}] = cdd_op[m][m][s];
                    for (uint16_t j = m + 1; j < n_sites; j++)
                        (*prmat)[{p + j - m, pp + j - m - 1}] = i_op;
                    p += n_sites - m, pp += n_sites - m - 1;
                }
                assert(pp == 1 + 4 * (n_sites - m - 1) +
                                 4 * 4 * (n_sites - m - 1 + 1) *
                                     (n_sites - m - 1) / 2 +
                                 2 * 8 * (n_sites - m - 1));
            }
            if (m == n_sites - 1) {
                // 1*16 : mmmm / ccdd (only last site)
                for (uint8_t s = 0; s < 16; s++)
                    (*prmat)[{p + s, 0}] = ccdd_op[m][s];
                p += 16;
            }
            assert(p == rlshape);
            opt->lmat = plmat, opt->rmat = prmat;
            hamil.filter_site_ops(m, {opt->lmat, opt->rmat}, opt->ops);
            this->tensors.push_back(opt);
        }
        // checking
        for (uint16_t m = 0; m < n_sites; m++) {
            if (m < n_sites - 1) {
                int mshape = (int)this->middle_operator_names[m]->data.size();
                auto pmop = dynamic_pointer_cast<SymbolicColumnVector<S>>(
                    this->middle_operator_names[m]);
                auto pmexpr = dynamic_pointer_cast<SymbolicColumnVector<S>>(
                    this->middle_operator_exprs[m]);
                for (int i = 0; i < mshape; i++) {
                    shared_ptr<OpElement<S>> op =
                        dynamic_pointer_cast<OpElement<S>>((*pmop)[i]);
                    shared_ptr<OpString<S>> ex =
                        dynamic_pointer_cast<OpString<S>>((*pmexpr)[i]);
                    assert(op->q_label == ex->a->q_label + ex->b->q_label);
                }
                if (m > 0) {
                    auto plmat = dynamic_pointer_cast<SymbolicMatrix<S>>(
                        this->tensors[m]->lmat);
                    for (size_t i = 0; i < plmat->data.size(); i++) {
                        shared_ptr<OpElement<S>> op =
                            dynamic_pointer_cast<OpElement<S>>(
                                this->left_operator_names[m]
                                    ->data[plmat->indices[i].second]);
                        if (plmat->data[i]->get_type() == OpTypes::Zero)
                            continue;
                        shared_ptr<OpElement<S>> exa =
                            dynamic_pointer_cast<OpElement<S>>(
                                this->left_operator_names[m - 1]
                                    ->data[plmat->indices[i].first]);
                        shared_ptr<OpElement<S>> exb =
                            dynamic_pointer_cast<OpElement<S>>(plmat->data[i]);
                        assert(op->q_label == exa->q_label + exb->q_label);
                    }
                } else if (m == 0) {
                    auto plmat = this->tensors[m]->lmat;
                    for (size_t i = 0; i < plmat->data.size(); i++) {
                        shared_ptr<OpElement<S>> op =
                            dynamic_pointer_cast<OpElement<S>>(
                                this->left_operator_names[m]->data[i]);
                        if (plmat->data[i]->get_type() == OpTypes::Zero)
                            continue;
                        shared_ptr<OpElement<S>> ex =
                            dynamic_pointer_cast<OpElement<S>>(plmat->data[i]);
                        assert(op->q_label == ex->q_label);
                    }
                }
            }
            if (m == n_sites - 1) {
                auto prmat = this->tensors[m]->rmat;
                for (size_t i = 0; i < prmat->data.size(); i++) {
                    shared_ptr<OpElement<S>> op =
                        dynamic_pointer_cast<OpElement<S>>(
                            this->right_operator_names[m]->data[i]);
                    if (prmat->data[i]->get_type() == OpTypes::Zero)
                        continue;
                    shared_ptr<OpElement<S>> ex =
                        dynamic_pointer_cast<OpElement<S>>(prmat->data[i]);
                    assert(op->q_label == ex->q_label);
                }
            } else if (m != 0) {
                auto prmat = dynamic_pointer_cast<SymbolicMatrix<S>>(
                    this->tensors[m]->rmat);
                for (size_t i = 0; i < prmat->data.size(); i++) {
                    shared_ptr<OpElement<S>> op =
                        dynamic_pointer_cast<OpElement<S>>(
                            this->right_operator_names[m]
                                ->data[prmat->indices[i].first]);
                    if (prmat->data[i]->get_type() == OpTypes::Zero)
                        continue;
                    shared_ptr<OpElement<S>> exb =
                        dynamic_pointer_cast<OpElement<S>>(
                            this->right_operator_names[m + 1]
                                ->data[prmat->indices[i].second]);
                    shared_ptr<OpElement<S>> exa =
                        dynamic_pointer_cast<OpElement<S>>(prmat->data[i]);
                    assert(op->q_label == exa->q_label + exb->q_label);
                }
            }
        }
    }
    void deallocate() override {
        for (uint16_t m = MPO<S>::n_sites - 1; m < MPO<S>::n_sites; m--)
            for (auto it = this->tensors[m]->ops.crbegin();
                 it != this->tensors[m]->ops.crend(); ++it) {
                OpElement<S> &op =
                    *dynamic_pointer_cast<OpElement<S>>(it->first);
                if (op.name == OpNames::CCDD || op.name == OpNames::CCD ||
                    op.name == OpNames::CDD)
                    it->second->deallocate();
            }
    }
};

} // namespace block2

#undef SI
#undef SJ
#undef SK
#undef SL
#undef PI
#undef PJ
#undef PK
#undef PL
#undef PIJ
#undef PIJK
