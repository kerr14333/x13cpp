// checkopt_cmn.hpp -- the check{} spec's own scalar options.
//
// Not a single Fortran COMMON: Mxcklg lives in /mdldg/ and Acflim/Qcheck in
// /model/ in the oracle. Grouped here because the port had no home for any of
// them -- `gt_check` routed the whole spec through `gt_generic`, so all three
// were parsed and dropped.
#ifndef X13_GEN_CHECKOPT_CMN_HPP
#define X13_GEN_CHECKOPT_CMN_HPP

namespace x13 {

struct checkopt_cmn {
    // gtinpt.f:320 -- 0 means UNSET, i.e. "derive it". getchk.f:63-67 sets
    // 10 (nonseasonal) or 2*Sp when check{} appears; gtinpt.f:1169 sets 3*Sp
    // for a SEATS run that left it 0; editor.f:909-910 sets 2*Sp for any model
    // run with Lsumm>0 that STILL left it 0 -- which is why the diagnostics
    // appear in every golden here, check{} present or not.
    int mxcklg = 0;
    // gtinpt.f:295-296. Qcheck's default is PT5 = 0.05D0 (gtinpt.f:21) -- a
    // probability, despite the name; the same trap as x11regression's Cvxalf.
    double acflim = 1.6;
    double qcheck = 0.05;
    // getchk.f:123-136 -- which Q the PRINTED table uses. The savelog block
    // acfdgn.f writes is unaffected: it always emits BOTH (lbq and bpq).
    int iqtype = 0;
};

}  // namespace x13

#endif  // X13_GEN_CHECKOPT_CMN_HPP
