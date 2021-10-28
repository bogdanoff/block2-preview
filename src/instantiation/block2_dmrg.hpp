
/*
 * block2: Efficient MPO implementation of quantum chemistry DMRG
 * Copyright (C) 2020-2021 Huanchen Zhai <hczhai@caltech.edu>
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

#include "../core/symmetry.hpp"
#include "../dmrg/archived_mpo.hpp"
#include "../dmrg/determinant.hpp"
#include "../dmrg/effective_hamiltonian.hpp"
#include "../dmrg/moving_environment.hpp"
#include "../dmrg/mpo.hpp"
#include "../dmrg/mpo_fusing.hpp"
#include "../dmrg/mpo_simplification.hpp"
#include "../dmrg/mps.hpp"
#include "../dmrg/mps_unfused.hpp"
#include "../dmrg/parallel_mpo.hpp"
#include "../dmrg/parallel_mps.hpp"
#include "../dmrg/parallel_rule_sum_mpo.hpp"
#include "../dmrg/partition.hpp"
#include "../dmrg/qc_hamiltonian.hpp"
#include "../dmrg/qc_mpo.hpp"
#include "../dmrg/qc_ncorr.hpp"
#include "../dmrg/qc_parallel_rule.hpp"
#include "../dmrg/qc_pdm1.hpp"
#include "../dmrg/qc_pdm2.hpp"
#include "../dmrg/qc_rule.hpp"
#include "../dmrg/qc_sum_mpo.hpp"
#include "../dmrg/state_averaged.hpp"
#include "../dmrg/sweep_algorithm.hpp"
#include "../dmrg/sweep_algorithm_td.hpp"
#include "block2_core.hpp"

// archived_mpo.hpp
extern template struct block2::ArchivedMPO<block2::SZ, double>;
extern template struct block2::ArchivedMPO<block2::SU2, double>;

// determinant.hpp
extern template struct block2::DeterminantTRIE<block2::SZ, double>;
extern template struct block2::DeterminantQC<block2::SZ, double>;
extern template struct block2::DeterminantMPSInfo<block2::SZ, double>;

extern template struct block2::DeterminantTRIE<block2::SU2, double>;
extern template struct block2::DeterminantQC<block2::SU2, double>;
extern template struct block2::DeterminantMPSInfo<block2::SU2, double>;

// effective_hamiltonian.hpp
extern template struct block2::EffectiveHamiltonian<
    block2::SZ, double, block2::MPS<block2::SZ, double>>;
extern template struct block2::LinearEffectiveHamiltonian<block2::SZ, double>;
extern template struct block2::EffectiveHamiltonian<
    block2::SZ, double, block2::MultiMPS<block2::SZ, double>>;

extern template struct block2::EffectiveHamiltonian<
    block2::SU2, double, block2::MPS<block2::SU2, double>>;
extern template struct block2::LinearEffectiveHamiltonian<block2::SU2, double>;
extern template struct block2::EffectiveHamiltonian<
    block2::SU2, double, block2::MultiMPS<block2::SU2, double>>;

// moving_environment.hpp
extern template struct block2::MovingEnvironment<block2::SZ, double, double>;
extern template struct block2::MovingEnvironment<block2::SU2, double, double>;

// mpo.hpp
extern template struct block2::MPOSchemer<block2::SZ>;
extern template struct block2::MPO<block2::SZ, double>;
extern template struct block2::DiagonalMPO<block2::SZ, double>;
extern template struct block2::AncillaMPO<block2::SZ, double>;
extern template struct block2::IdentityAddedMPO<block2::SZ, double>;

extern template struct block2::MPOSchemer<block2::SU2>;
extern template struct block2::MPO<block2::SU2, double>;
extern template struct block2::DiagonalMPO<block2::SU2, double>;
extern template struct block2::AncillaMPO<block2::SU2, double>;
extern template struct block2::IdentityAddedMPO<block2::SU2, double>;

// mpo_fusing.hpp
extern template struct block2::FusedMPO<block2::SZ, double>;
extern template struct block2::FusedMPO<block2::SU2, double>;

// mpo_simplification.hpp
extern template struct block2::SimplifiedMPO<block2::SZ, double>;
extern template struct block2::SimplifiedMPO<block2::SU2, double>;

// mps.hpp
extern template struct block2::MPSInfo<block2::SZ>;
extern template struct block2::DynamicMPSInfo<block2::SZ>;
extern template struct block2::CASCIMPSInfo<block2::SZ>;
extern template struct block2::MRCIMPSInfo<block2::SZ>;
extern template struct block2::AncillaMPSInfo<block2::SZ>;
extern template struct block2::MPS<block2::SZ, double>;

extern template struct block2::MPSInfo<block2::SU2>;
extern template struct block2::DynamicMPSInfo<block2::SU2>;
extern template struct block2::CASCIMPSInfo<block2::SU2>;
extern template struct block2::MRCIMPSInfo<block2::SU2>;
extern template struct block2::AncillaMPSInfo<block2::SU2>;
extern template struct block2::MPS<block2::SU2, double>;

extern template struct block2::TransMPSInfo<block2::SZ, block2::SU2>;
extern template struct block2::TransMPSInfo<block2::SU2, block2::SZ>;

// mps_unfused.hpp
extern template struct block2::SparseTensor<block2::SZ, double>;
extern template struct block2::UnfusedMPS<block2::SZ, double>;

extern template struct block2::SparseTensor<block2::SU2, double>;
extern template struct block2::UnfusedMPS<block2::SU2, double>;

// parallel_mpo.hpp
extern template struct block2::ClassicParallelMPO<block2::SZ, double>;
extern template struct block2::ParallelMPO<block2::SZ, double>;

extern template struct block2::ClassicParallelMPO<block2::SU2, double>;
extern template struct block2::ParallelMPO<block2::SU2, double>;

// parallel_mps.hpp
extern template struct block2::ParallelMPS<block2::SZ, double>;
extern template struct block2::ParallelMPS<block2::SU2, double>;

// parallel_rule_sum_mpo.hpp
extern template struct block2::ParallelRuleSumMPO<block2::SZ, double>;
extern template struct block2::SumMPORule<block2::SZ, double>;
extern template struct block2::ParallelFCIDUMP<block2::SZ, double>;

extern template struct block2::ParallelRuleSumMPO<block2::SU2, double>;
extern template struct block2::SumMPORule<block2::SU2, double>;
extern template struct block2::ParallelFCIDUMP<block2::SU2, double>;

// partition.hpp
extern template struct block2::Partition<block2::SZ, double>;
extern template struct block2::Partition<block2::SU2, double>;

// qc_hamiltonian.hpp
extern template struct block2::HamiltonianQC<block2::SZ, double>;
extern template struct block2::HamiltonianQC<block2::SU2, double>;

// qc_mpo.hpp
extern template struct block2::IdentityMPO<block2::SZ, double>;
extern template struct block2::SiteMPO<block2::SZ, double>;
extern template struct block2::MPOQC<block2::SZ, double>;

extern template struct block2::IdentityMPO<block2::SU2, double>;
extern template struct block2::SiteMPO<block2::SU2, double>;
extern template struct block2::MPOQC<block2::SU2, double>;

// qc_ncorr.hpp
extern template struct block2::NPC1MPOQC<block2::SZ, double>;
extern template struct block2::NPC1MPOQC<block2::SU2, double>;

// qc_parallel_rule.hpp
extern template struct block2::ParallelRuleQC<block2::SZ, double>;
extern template struct block2::ParallelRuleOneBodyQC<block2::SZ, double>;
extern template struct block2::ParallelRulePDM1QC<block2::SZ, double>;
extern template struct block2::ParallelRulePDM2QC<block2::SZ, double>;
extern template struct block2::ParallelRuleNPDMQC<block2::SZ, double>;
extern template struct block2::ParallelRuleSiteQC<block2::SZ, double>;
extern template struct block2::ParallelRuleIdentity<block2::SZ, double>;

extern template struct block2::ParallelRuleQC<block2::SU2, double>;
extern template struct block2::ParallelRuleOneBodyQC<block2::SU2, double>;
extern template struct block2::ParallelRulePDM1QC<block2::SU2, double>;
extern template struct block2::ParallelRulePDM2QC<block2::SU2, double>;
extern template struct block2::ParallelRuleNPDMQC<block2::SU2, double>;
extern template struct block2::ParallelRuleSiteQC<block2::SU2, double>;
extern template struct block2::ParallelRuleIdentity<block2::SU2, double>;

// qc_pdm1.hpp
extern template struct block2::PDM1MPOQC<block2::SZ, double>;
extern template struct block2::PDM1MPOQC<block2::SU2, double>;

// qc_pdm2.hpp
extern template struct block2::PDM2MPOQC<block2::SZ, double>;
extern template struct block2::PDM2MPOQC<block2::SU2, double>;

// qc_rule.hpp
extern template struct block2::RuleQC<block2::SZ, double>;
extern template struct block2::AntiHermitianRuleQC<block2::SZ, double>;

extern template struct block2::RuleQC<block2::SU2, double>;
extern template struct block2::AntiHermitianRuleQC<block2::SU2, double>;

// qc_sum_mpo.hpp
extern template struct block2::SumMPOQC<block2::SZ, double>;

// state_averaged.hpp
extern template struct block2::MultiMPSInfo<block2::SZ>;
extern template struct block2::MultiMPS<block2::SZ, double>;

extern template struct block2::MultiMPSInfo<block2::SU2>;
extern template struct block2::MultiMPS<block2::SU2, double>;

// sweep_algorithm.hpp
extern template struct block2::DMRG<block2::SZ, double, double>;
extern template struct block2::Linear<block2::SZ, double, double>;
extern template struct block2::Expect<block2::SZ, double, double, double>;
extern template struct block2::Expect<block2::SZ, double, double, complex<double>>;

extern template struct block2::DMRG<block2::SU2, double, double>;
extern template struct block2::Linear<block2::SU2, double, double>;
extern template struct block2::Expect<block2::SU2, double, double, double>;
extern template struct block2::Expect<block2::SU2, double, double, complex<double>>;

// sweep_algorithm_td.hpp
extern template struct block2::TDDMRG<block2::SZ, double, double>;
extern template struct block2::TimeEvolution<block2::SZ, double, double>;

extern template struct block2::TDDMRG<block2::SU2, double, double>;
extern template struct block2::TimeEvolution<block2::SU2, double, double>;

#ifdef _USE_KSYMM

// archived_mpo.hpp
extern template struct block2::ArchivedMPO<block2::SZK, double>;
extern template struct block2::ArchivedMPO<block2::SU2K, double>;

// determinant.hpp
extern template struct block2::DeterminantTRIE<block2::SZK, double>;
extern template struct block2::DeterminantQC<block2::SZK, double>;
extern template struct block2::DeterminantMPSInfo<block2::SZK, double>;

extern template struct block2::DeterminantTRIE<block2::SU2K, double>;
extern template struct block2::DeterminantQC<block2::SU2K, double>;
extern template struct block2::DeterminantMPSInfo<block2::SU2K, double>;

// effective_hamiltonian.hpp
extern template struct block2::EffectiveHamiltonian<
    block2::SZK, double, block2::MPS<block2::SZK, double>>;
extern template struct block2::LinearEffectiveHamiltonian<block2::SZK, double>;
extern template struct block2::EffectiveHamiltonian<
    block2::SZK, double, block2::MultiMPS<block2::SZK, double>>;

extern template struct block2::EffectiveHamiltonian<
    block2::SU2K, double, block2::MPS<block2::SU2K, double>>;
extern template struct block2::LinearEffectiveHamiltonian<block2::SU2K, double>;
extern template struct block2::EffectiveHamiltonian<
    block2::SU2K, double, block2::MultiMPS<block2::SU2K, double>>;

// moving_environment.hpp
extern template struct block2::MovingEnvironment<block2::SZK, double, double>;
extern template struct block2::MovingEnvironment<block2::SU2K, double, double>;

// mpo.hpp
extern template struct block2::MPOSchemer<block2::SZK>;
extern template struct block2::MPO<block2::SZK, double>;
extern template struct block2::DiagonalMPO<block2::SZK, double>;
extern template struct block2::AncillaMPO<block2::SZK, double>;
extern template struct block2::IdentityAddedMPO<block2::SZK, double>;

extern template struct block2::MPOSchemer<block2::SU2K>;
extern template struct block2::MPO<block2::SU2K, double>;
extern template struct block2::DiagonalMPO<block2::SU2K, double>;
extern template struct block2::AncillaMPO<block2::SU2K, double>;
extern template struct block2::IdentityAddedMPO<block2::SU2K, double>;

// mpo_fusing.hpp
extern template struct block2::FusedMPO<block2::SZK, double>;
extern template struct block2::FusedMPO<block2::SU2K, double>;

// mpo_simplification.hpp
extern template struct block2::SimplifiedMPO<block2::SZK, double>;
extern template struct block2::SimplifiedMPO<block2::SU2K, double>;

// mps.hpp
extern template struct block2::MPSInfo<block2::SZK>;
extern template struct block2::DynamicMPSInfo<block2::SZK>;
extern template struct block2::CASCIMPSInfo<block2::SZK>;
extern template struct block2::MRCIMPSInfo<block2::SZK>;
extern template struct block2::AncillaMPSInfo<block2::SZK>;
extern template struct block2::MPS<block2::SZK, double>;

extern template struct block2::MPSInfo<block2::SU2K>;
extern template struct block2::DynamicMPSInfo<block2::SU2K>;
extern template struct block2::CASCIMPSInfo<block2::SU2K>;
extern template struct block2::MRCIMPSInfo<block2::SU2K>;
extern template struct block2::AncillaMPSInfo<block2::SU2K>;
extern template struct block2::MPS<block2::SU2K, double>;

extern template struct block2::TransMPSInfo<block2::SZK, block2::SU2K>;
extern template struct block2::TransMPSInfo<block2::SU2K, block2::SZK>;

// mps_unfused.hpp
extern template struct block2::SparseTensor<block2::SZK, double>;
extern template struct block2::UnfusedMPS<block2::SZK, double>;

extern template struct block2::SparseTensor<block2::SU2K, double>;
extern template struct block2::UnfusedMPS<block2::SU2K, double>;

// parallel_mpo.hpp
extern template struct block2::ClassicParallelMPO<block2::SZK, double>;
extern template struct block2::ParallelMPO<block2::SZK, double>;

extern template struct block2::ClassicParallelMPO<block2::SU2K, double>;
extern template struct block2::ParallelMPO<block2::SU2K, double>;

// parallel_mps.hpp
extern template struct block2::ParallelMPS<block2::SZK, double>;
extern template struct block2::ParallelMPS<block2::SU2K, double>;

// parallel_rule_sum_mpo.hpp
extern template struct block2::ParallelRuleSumMPO<block2::SZK, double>;
extern template struct block2::SumMPORule<block2::SZK, double>;
extern template struct block2::ParallelFCIDUMP<block2::SZK, double>;

extern template struct block2::ParallelRuleSumMPO<block2::SU2K, double>;
extern template struct block2::SumMPORule<block2::SU2K, double>;
extern template struct block2::ParallelFCIDUMP<block2::SU2K, double>;

// partition.hpp
extern template struct block2::Partition<block2::SZK, double>;
extern template struct block2::Partition<block2::SU2K, double>;

// qc_hamiltonian.hpp
extern template struct block2::HamiltonianQC<block2::SZK, double>;
extern template struct block2::HamiltonianQC<block2::SU2K, double>;

// qc_mpo.hpp
extern template struct block2::IdentityMPO<block2::SZK, double>;
extern template struct block2::SiteMPO<block2::SZK, double>;
extern template struct block2::MPOQC<block2::SZK, double>;

extern template struct block2::IdentityMPO<block2::SU2K, double>;
extern template struct block2::SiteMPO<block2::SU2K, double>;
extern template struct block2::MPOQC<block2::SU2K, double>;

// qc_ncorr.hpp
extern template struct block2::NPC1MPOQC<block2::SZK, double>;
extern template struct block2::NPC1MPOQC<block2::SU2K, double>;

// qc_parallel_rule.hpp
extern template struct block2::ParallelRuleQC<block2::SZK, double>;
extern template struct block2::ParallelRuleOneBodyQC<block2::SZK, double>;
extern template struct block2::ParallelRulePDM1QC<block2::SZK, double>;
extern template struct block2::ParallelRulePDM2QC<block2::SZK, double>;
extern template struct block2::ParallelRuleNPDMQC<block2::SZK, double>;
extern template struct block2::ParallelRuleSiteQC<block2::SZK, double>;
extern template struct block2::ParallelRuleIdentity<block2::SZK, double>;

extern template struct block2::ParallelRuleQC<block2::SU2K, double>;
extern template struct block2::ParallelRuleOneBodyQC<block2::SU2K, double>;
extern template struct block2::ParallelRulePDM1QC<block2::SU2K, double>;
extern template struct block2::ParallelRulePDM2QC<block2::SU2K, double>;
extern template struct block2::ParallelRuleNPDMQC<block2::SU2K, double>;
extern template struct block2::ParallelRuleSiteQC<block2::SU2K, double>;
extern template struct block2::ParallelRuleIdentity<block2::SU2K, double>;

// qc_pdm1.hpp
extern template struct block2::PDM1MPOQC<block2::SZK, double>;
extern template struct block2::PDM1MPOQC<block2::SU2K, double>;

// qc_pdm2.hpp
extern template struct block2::PDM2MPOQC<block2::SZK, double>;
extern template struct block2::PDM2MPOQC<block2::SU2K, double>;

// qc_rule.hpp
extern template struct block2::RuleQC<block2::SZK, double>;
extern template struct block2::AntiHermitianRuleQC<block2::SZK, double>;

extern template struct block2::RuleQC<block2::SU2K, double>;
extern template struct block2::AntiHermitianRuleQC<block2::SU2K, double>;

// qc_sum_mpo.hpp
extern template struct block2::SumMPOQC<block2::SZK, double>;

// state_averaged.hpp
extern template struct block2::MultiMPSInfo<block2::SZK>;
extern template struct block2::MultiMPS<block2::SZK, double>;

extern template struct block2::MultiMPSInfo<block2::SU2K>;
extern template struct block2::MultiMPS<block2::SU2K, double>;

// sweep_algorithm.hpp
extern template struct block2::DMRG<block2::SZK, double, double>;
extern template struct block2::Linear<block2::SZK, double, double>;
extern template struct block2::Expect<block2::SZK, double, double, double>;
extern template struct block2::Expect<block2::SZK, double, double, complex<double>>;

extern template struct block2::DMRG<block2::SU2K, double, double>;
extern template struct block2::Linear<block2::SU2K, double, double>;
extern template struct block2::Expect<block2::SU2K, double, double, double>;
extern template struct block2::Expect<block2::SU2K, double, double, complex<double>>;

// sweep_algorithm_td.hpp
extern template struct block2::TDDMRG<block2::SZK, double, double>;
extern template struct block2::TimeEvolution<block2::SZK, double, double>;

extern template struct block2::TDDMRG<block2::SU2K, double, double>;
extern template struct block2::TimeEvolution<block2::SU2K, double, double>;

#endif
