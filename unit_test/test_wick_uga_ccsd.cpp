
#include "ic/wick.hpp"
#include <gtest/gtest.h>

using namespace block2;

class TestWickUGACCSD : public ::testing::Test {
  protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TestWickUGACCSD, TestUGACCSD) {
    WickUGACCSD wccsd;

    // See J. Chem. Phys. 89, 7382 (1988)
    wccsd.defs["tau"] = WickExpr::parse_def(
        "tau[abij] = t[abij] \n + t[ai] t[bj]", wccsd.idx_map, wccsd.perm_map);
    wccsd.defs["T"] =
        WickExpr::parse_def("T[abij] = 0.5 t[abij] \n + t[ai] t[bj]",
                            wccsd.idx_map, wccsd.perm_map);

    wccsd.defs["hi"] = WickExpr::parse_def(R"TEX(hi[ik] =
 + 1.0 f[ik]
 + 2.0 SUM <jab> v[ijab] tau[abkj]
 - 1.0 SUM <jab> v[ijba] tau[abkj]
)TEX",
                                           wccsd.idx_map, wccsd.perm_map);

    wccsd.defs["ha"] = WickExpr::parse_def(R"TEX(ha[ca] =
 + 1.0 f[ca]
 - 2.0 SUM <ijb> v[ijab] tau[cbij]
 + 1.0 SUM <ijb> v[ijba] tau[cbij]
)TEX",
                                           wccsd.idx_map, wccsd.perm_map);

    wccsd.defs["hia"] = WickExpr::parse_def(R"TEX(hia[ia] =
 + 1.0 f[ia]
 + 2.0 SUM <bj> v[ijab] t[bj]
 - 1.0 SUM <bj> v[ijba] t[bj]
)TEX",
                                            wccsd.idx_map, wccsd.perm_map);

    wccsd.defs["hi"].second = wccsd.defs["hi"].second.substitute(wccsd.defs);
    wccsd.defs["ha"].second = wccsd.defs["ha"].second.substitute(wccsd.defs);

    WickExpr e_ref = WickExpr::parse(R"TEX(
 + 2.0\sum_{ia}f_{ia}t_{ai}
 + 2.0\sum_{aibj}v_{ijab}tau_{abij}
 - 1.0\sum_{aibj}v_{ijba}tau_{abij}
)TEX",
                                     wccsd.idx_map, wccsd.perm_map)
                         .substitute(wccsd.defs);

    WickExpr t1_ref = 2.0 * WickExpr::parse(R"TEX(
 + 1.0 f[ai]
 - 2.0 SUM <jb> f[jb] t[aj] t[bi]
 + 1.0 SUM <b> ha[ab] t[bi]
 - 1.0 SUM <j> hi[ji] t[aj]
 + 2.0 SUM <jb> hia[jb] t[baji]
 - 1.0 SUM <jb> hia[jb] t[baij]
 + 1.0 SUM <jb> hia[jb] t[bi] t[aj]
 + 2.0 SUM <bj> v[jabi] t[bj]
 - 1.0 SUM <bj> v[jaib] t[bj]
 + 2.0 SUM <bcj> v[jabc] tau[bcji]
 - 1.0 SUM <bcj> v[jacb] tau[bcji]
 - 2.0 SUM <bjk> v[jkbi] tau[bajk]
 + 1.0 SUM <bjk> v[kjbi] tau[bajk]
)TEX",
                                            wccsd.idx_map, wccsd.perm_map)
                                .substitute(wccsd.defs)
                                .simplify();

    wccsd.defs["gi"] = WickExpr::parse_def(R"TEX(gi[ki] =
 + 1.0 hi[ki]
 + 1.0 SUM <c> f[kc] t[ci]
 + 2.0 SUM <lc> v[klic] t[cl]
 - 1.0 SUM <lc> v[lkic] t[cl]
)TEX",
                                           wccsd.idx_map, wccsd.perm_map);
    wccsd.defs["ga"] = WickExpr::parse_def(R"TEX(ga[ac] =
 + 1.0 ha[ac]
 - 1.0 SUM <k> f[kc] t[ak]
 + 2.0 SUM <kd> v[akcd] t[dk]
 - 1.0 SUM <kd> v[akdc] t[dk]
)TEX",
                                           wccsd.idx_map, wccsd.perm_map);
    wccsd.defs["a"] = WickExpr::parse_def(R"TEX(a[klij] =
 + 1.0 v[klij]
 + 1.0 SUM <c> v[klic] t[cj]
 + 1.0 SUM <c> v[klcj] t[ci]
 + 1.0 SUM <cd> v[klcd] tau[cdij]
)TEX",
                                          wccsd.idx_map, wccsd.perm_map);
    wccsd.defs["b"] = WickExpr::parse_def(R"TEX(b[abcd] =
 + 1.0 v[abcd]
 - 1.0 SUM <k> v[akcd] t[bk]
 - 1.0 SUM <k> v[kbcd] t[ak]
)TEX",
                                          wccsd.idx_map, wccsd.perm_map);
    wccsd.defs["j"] = WickExpr::parse_def(R"TEX(j[akic] =
 + 1.0 v[akic]
 - 1.0 SUM <l> v[lkic] t[al]
 + 1.0 SUM <d> v[akdc] t[di]
 - 1.0 SUM <dl> v[klcd] T[dail]
 + 1.0 SUM <dl> v[klcd] t[adil]
 - 0.5 SUM <dl> v[kldc] t[adil]
)TEX",
                                          wccsd.idx_map, wccsd.perm_map);
    wccsd.defs["k"] = WickExpr::parse_def(R"TEX(k[kaic] =
 + 1.0 v[kaic]
 - 1.0 SUM <l> v[klic] t[al]
 + 1.0 SUM <d> v[kadc] t[di]
 - 1.0 SUM <dl> v[kldc] T[dail]
)TEX",
                                          wccsd.idx_map, wccsd.perm_map);

    wccsd.defs["gi"].second = wccsd.defs["gi"].second.substitute(wccsd.defs);
    wccsd.defs["ga"].second = wccsd.defs["ga"].second.substitute(wccsd.defs);
    wccsd.defs["a"].second = wccsd.defs["a"].second.substitute(wccsd.defs);
    wccsd.defs["b"].second = wccsd.defs["b"].second.substitute(wccsd.defs);
    wccsd.defs["j"].second = wccsd.defs["j"].second.substitute(wccsd.defs);
    wccsd.defs["k"].second = wccsd.defs["k"].second.substitute(wccsd.defs);

    WickExpr t2_uga_a = WickExpr::parse(R"TEX(
 + 1.0 v[abij]
 + 1.0 SUM <kl> a[klij] tau[abkl]
 + 1.0 SUM <cd> b[abcd] tau[cdij]
)TEX",
                                        wccsd.idx_map, wccsd.perm_map);
    WickExpr t2_uga_b = WickExpr::parse(R"TEX(
 + 1.0 SUM <c> ga[ac] t[cbij]
 - 1.0 SUM <k> gi[ki] t[abkj]
 + 1.0 SUM <c> v[abic] t[cj]
 - 1.0 SUM <ck> v[kbic] t[ak] t[cj]
 - 1.0 SUM <k> v[akij] t[bk]
 - 1.0 SUM <ck> v[akic] t[cj] t[bk]
 + 2.0 SUM <ck> j[akic] t[cbkj]
 - 1.0 SUM <ck> j[akic] t[bckj]
 - 1.0 SUM <ck> k[kaic] t[cbkj]
 - 1.0 SUM <ck> k[kbic] t[ackj]
)TEX",
                                        wccsd.idx_map, wccsd.perm_map);

    map<string, string> t2_maps{make_pair("a", "b"), make_pair("b", "a"),
                                make_pair("i", "j"), make_pair("j", "i")};
    WickExpr t2_uga = (t2_uga_a + t2_uga_b + t2_uga_b.index_map(t2_maps))
                          .substitute(wccsd.defs)
                          .simplify();

    vector<WickExpr> t2_ref(5);
    for (auto &ws : t2_uga.terms) {
        int t_count = 0;
        for (auto &wt : ws.tensors)
            if (wt.name == "t")
                t_count++;
        assert(t_count >= 0 && t_count < t2_ref.size());
        t2_ref[t_count].terms.push_back(ws);
    }

    map<string, string> uga_maps{make_pair("i", "j"), make_pair("j", "i")};
    for (auto &r : t2_ref)
        r = (4 * r - 2 * r.index_map(uga_maps)).simplify();

    WickExpr e_eq = wccsd.energy_equations();
    // cout << e_eq << endl;
    // cout << e_ref << endl;
    WickExpr diff_e = (e_eq - e_ref).simplify();
    cout << "DIFF E = " << diff_e << endl;
    EXPECT_TRUE(diff_e.terms.size() == 0);
    WickExpr t1_eq = wccsd.t1_equations();
    // cout << t1_eq << endl;
    // cout << t1_ref << endl;
    WickExpr diff = (t1_eq - t1_ref).simplify();
    cout << "DIFF T1 = " << diff << endl;
    EXPECT_TRUE(diff.terms.size() == 0);
    for (int i = 0; i <= 4; i++) {
        WickExpr t2_eq = wccsd.t2_equations(i);
        WickExpr x_t2_ref = t2_ref[0];
        for (int j = 1; j <= i; j++)
            x_t2_ref = x_t2_ref + t2_ref[j];
        // cout << t2_eq << endl;
        // cout << x_t2_ref << endl;
        WickExpr diff = (t2_eq - x_t2_ref).simplify();
        cout << "DIFF T2 (order = " << i << ") = " << diff << endl;
        EXPECT_TRUE(diff.terms.size() == 0);
    }
}
