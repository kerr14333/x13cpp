// agr.hpp -- composite (aggregate/indirect) adjustment primitives.
//
// Ports agr.f (the accumulate primitive) and agr1.f (initialize / hand the
// composite total to the composite{} spec). See tools/composite_scouting.md for
// the whole-feature map.
//
// Composite adjustment is the one X-13 feature that spans MULTIPLE spec runs:
// the oracle runs every spec of a metafile in one process, so the aggregation
// COMMONs (/mq11/ = agr.cmn, /agreg/ = agrsrs.cmn) persist across runs and THAT
// persistence is the feature. In this port the caller (the metafile driver)
// carries agr_cmn + agrsrs_cmn from one X13Context to the next.
#ifndef X13_COMPOSITE_AGR_HPP
#define X13_COMPOSITE_AGR_HPP

namespace x13 {

struct X13Context;

// agr.f -- composite one component series into an accumulator:
//   B(j) = B(j) (+|-|*|/) (A(i)*Wt),  j = i + j0 - j1,  i = j1..j2
// iag: 0=add 1=sub 2=mult 3=div (series{comptype} minus 2). wt==0 is treated as
// 1 (and, faithfully, the Fortran WRITES that back through its by-reference Wt).
// a/b are 1-based PLEN buffers.
void agr(const double* a, double* b, int iag, int j1, int j2, int j0, double& wt);

// agr1.f -- two jobs, selected by Iagr:
//   Iagr == 0 : INITIALIZE. Zero O..Ci/Omod, Ncomp/Nrcomp/Nscomp, the indirect
//               sliding-spans (Saind/Sfind/Sfinda) and history (Cncisa/Finisa)
//               buffers, and the Lind* flags; set Iagr=1.
//   Iagr  > 0 : hand the accumulated direct total to the composite{} spec --
//               set Iagr=3, compute the observation count from Itest(1..5), and
//               copy Y(i) = O(i + Ind1ob - 1).
// `y` is the 1-based series buffer (ctx.arima.y); `nobs` is out.
void agr1(X13Context& ctx, double* y, int& nobs);

}  // namespace x13
#endif  // X13_COMPOSITE_AGR_HPP
