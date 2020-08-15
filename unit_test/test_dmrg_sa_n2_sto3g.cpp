
#include "block2.hpp"
#include <gtest/gtest.h>

using namespace block2;

class TestDMRGN2STO3GSA : public ::testing::Test {
  protected:
    size_t isize = 1L << 30;
    size_t dsize = 1L << 34;
    void SetUp() override {
        Random::rand_seed(0);
        frame_() = make_shared<DataFrame>(isize, dsize, "nodex");
    }
    void TearDown() override {
        frame_()->activate(0);
        assert(ialloc_()->used == 0 && dalloc_()->used == 0);
        frame_() = nullptr;
    }
};

TEST_F(TestDMRGN2STO3GSA, TestSU2) {
    shared_ptr<FCIDUMP> fcidump = make_shared<FCIDUMP>();
    PGTypes pg = PGTypes::D2H;
    string filename = "data/N2.STO3G.FCIDUMP";
    fcidump->read(filename);
    vector<uint8_t> orbsym = fcidump->orb_sym();
    transform(orbsym.begin(), orbsym.end(), orbsym.begin(),
              PointGroup::swap_pg(pg));

#ifdef _HAS_INTEL_MKL
    mkl_set_num_threads(8);
    mkl_set_dynamic(0);
#endif

    SU2 vacuum(0);

    vector<SU2> targets;
    int ne = fcidump->n_elec() / 2;
    for (int i = 0; i < 8; i++)
        for (int na = ne - 1; na <= ne + 1; na++)
            for (int nb = ne - 1; nb <= ne + 1; nb++)
                if (na - nb >= 0)
                    targets.push_back(SU2(na + nb, na - nb, i));

    vector<double> energies = {
        -107.654122447525, // < N=14 S=0 PG=0 >
        -107.356943001688, // < N=14 S=1 PG=2|3 >
        -107.356943001688, // < N=14 S=1 PG=2|3 >
        -107.343458537273, // < N=14 S=1 PG=5 >
        -107.319813793867, // < N=15 S=1/2 PG=2|3 >
        -107.319813793866, // < N=15 S=1/2 PG=2|3 >
        -107.306744734757, // < N=14 S=0 PG=2|3 >
        -107.306744734756, // < N=14 S=0 PG=2|3 >
        -107.279409754727, // < N=14 S=1 PG=4|5 >
        -107.279409754727  // < N=14 S=1 PG=4|5 >
    };

    int norb = fcidump->n_sites();
    HamiltonianQC<SU2> hamil(vacuum, norb, orbsym, fcidump);

    hamil.opf->seq->mode = SeqTypes::Simple;

    Timer t;
    t.get_time();
    // MPO construction
    cout << "MPO start" << endl;
    shared_ptr<MPO<SU2>> mpo =
        make_shared<MPOQC<SU2>>(hamil, QCTypes::Conventional);
    cout << "MPO end .. T = " << t.get_time() << endl;

    // MPO simplification
    cout << "MPO simplification start" << endl;
    mpo =
        make_shared<SimplifiedMPO<SU2>>(mpo, make_shared<RuleQC<SU2>>(), true);
    cout << "MPO simplification end .. T = " << t.get_time() << endl;

    uint16_t bond_dim = 200;
    uint16_t nroots = 10;
    vector<uint16_t> bdims = {bond_dim};
    vector<double> noises = {1E-6, 0.0};

    t.get_time();

    shared_ptr<MultiMPSInfo<SU2>> mps_info = make_shared<MultiMPSInfo<SU2>>(
        norb, vacuum, targets, hamil.basis, hamil.orb_sym);
    mps_info->set_bond_dimension(bond_dim);

    // MPS
    Random::rand_seed(0);

    shared_ptr<MultiMPS<SU2>> mps =
        make_shared<MultiMPS<SU2>>(norb, 0, 2, nroots);
    mps->initialize(mps_info);
    mps->random_canonicalize();

    // MPS/MPSInfo save mutable
    mps->save_mutable();
    mps->deallocate();
    mps_info->save_mutable();
    mps_info->deallocate_mutable();

    // ME
    shared_ptr<MovingEnvironment<SU2>> me =
        make_shared<MovingEnvironment<SU2>>(mpo, mps, mps, "DMRG");
    me->init_environments(false);

    // DMRG
    shared_ptr<DMRG<SU2>> dmrg = make_shared<DMRG<SU2>>(me, bdims, noises);
    dmrg->iprint = 1;
    double energy = dmrg->solve(10, true, 1E-8);

    for (size_t i = 0; i < dmrg->energies.back().size(); i++) {
        cout << "== SU2 (SA) =="
             << " E[" << setw(2) << i << "] = " << fixed << setw(22)
             << setprecision(12) << dmrg->energies.back()[i]
             << " error = " << scientific << setprecision(3) << setw(10)
             << (dmrg->energies.back()[i] - energies[i]) << endl;

        EXPECT_LT(abs(dmrg->energies.back()[i] - energies[i]), 1E-7);
    }

    // deallocate persistent stack memory
    mps_info->deallocate();

    mpo->deallocate();
    hamil.deallocate();
    fcidump->deallocate();
}

TEST_F(TestDMRGN2STO3GSA, TestSZ) {
    shared_ptr<FCIDUMP> fcidump = make_shared<FCIDUMP>();
    PGTypes pg = PGTypes::D2H;
    string filename = "data/N2.STO3G.FCIDUMP";
    fcidump->read(filename);
    vector<uint8_t> orbsym = fcidump->orb_sym();
    transform(orbsym.begin(), orbsym.end(), orbsym.begin(),
              PointGroup::swap_pg(pg));

#ifdef _HAS_INTEL_MKL
    mkl_set_num_threads(8);
    mkl_set_dynamic(0);
#endif

    SZ vacuum(0);

    vector<SZ> targets;
    int ne = fcidump->n_elec() / 2;
    for (int i = 0; i < 8; i++)
        for (int na = ne - 1; na <= ne + 1; na++)
            for (int nb = ne - 1; nb <= ne + 1; nb++)
                targets.push_back(SZ(na + nb, na - nb, i));

    vector<double> energies = {
        -107.654122447526, // < N=14 S=0 PG=0 >
        -107.356943001689, // < N=14 S=-1|0|1 PG=2|3 >
        -107.356943001688, // < N=14 S=-1|0|1 PG=2|3 >
        -107.356943001688, // < N=14 S=-1|0|1 PG=2|3 >
        -107.356943001688, // < N=14 S=-1|0|1 PG=2|3 >
        -107.356943001688, // < N=14 S=-1|0|1 PG=2|3 >
        -107.356943001688, // < N=14 S=-1|0|1 PG=2|3 >
        -107.343458537273, // < N=14 S=-1|0|1 PG=5 >
        -107.343458537273, // < N=14 S=-1|0|1 PG=5 >
        -107.343458537272, // < N=14 S=-1|0|1 PG=5 >
        -107.319813793867, // < N=15 S=-1/2|1/2 PG=2|3 >
        -107.319813793866, // < N=15 S=-1/2|1/2 PG=2|3 >
        -107.319813793866, // < N=15 S=-1/2|1/2 PG=2|3 >
        -107.319813793866, // < N=15 S=-1/2|1/2 PG=2|3 >
        -107.306744734756, // < N=14 S=0 PG=2|3 >
        -107.306744734756  // < N=14 S=0 PG=2|3 >
    };

    int norb = fcidump->n_sites();
    HamiltonianQC<SZ> hamil(vacuum, norb, orbsym, fcidump);

    hamil.opf->seq->mode = SeqTypes::Simple;

    Timer t;
    t.get_time();
    // MPO construction
    cout << "MPO start" << endl;
    shared_ptr<MPO<SZ>> mpo =
        make_shared<MPOQC<SZ>>(hamil, QCTypes::Conventional);
    cout << "MPO end .. T = " << t.get_time() << endl;

    // MPO simplification
    cout << "MPO simplification start" << endl;
    mpo = make_shared<SimplifiedMPO<SZ>>(mpo, make_shared<RuleQC<SZ>>(), true);
    cout << "MPO simplification end .. T = " << t.get_time() << endl;

    uint16_t bond_dim = 400;
    uint16_t nroots = 16;
    vector<uint16_t> bdims = {bond_dim};
    vector<double> noises = {1E-6, 0.0};

    t.get_time();

    shared_ptr<MultiMPSInfo<SZ>> mps_info = make_shared<MultiMPSInfo<SZ>>(
        norb, vacuum, targets, hamil.basis, hamil.orb_sym);
    mps_info->set_bond_dimension(bond_dim);

    // MPS
    Random::rand_seed(0);

    shared_ptr<MultiMPS<SZ>> mps =
        make_shared<MultiMPS<SZ>>(norb, 0, 2, nroots);
    mps->initialize(mps_info);
    mps->random_canonicalize();

    // MPS/MPSInfo save mutable
    mps->save_mutable();
    mps->deallocate();
    mps_info->save_mutable();
    mps_info->deallocate_mutable();

    // ME
    shared_ptr<MovingEnvironment<SZ>> me =
        make_shared<MovingEnvironment<SZ>>(mpo, mps, mps, "DMRG");
    me->init_environments(false);

    // DMRG
    shared_ptr<DMRG<SZ>> dmrg = make_shared<DMRG<SZ>>(me, bdims, noises);
    dmrg->iprint = 1;
    double energy = dmrg->solve(10, true, 1E-8);

    for (size_t i = 0; i < dmrg->energies.back().size(); i++) {
        cout << "== SU2 (SA) =="
             << " E[" << setw(2) << i << "] = " << fixed << setw(22)
             << setprecision(12) << dmrg->energies.back()[i]
             << " error = " << scientific << setprecision(3) << setw(10)
             << (dmrg->energies.back()[i] - energies[i]) << endl;

        EXPECT_LT(abs(dmrg->energies.back()[i] - energies[i]), 1E-7);
    }

    // deallocate persistent stack memory
    mps_info->deallocate();

    mpo->deallocate();
    hamil.deallocate();
    fcidump->deallocate();
}
