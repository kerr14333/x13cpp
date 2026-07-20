// x11drv.hpp -- X-11 Tier 4 driver routines: the per-pass compute drivers that
// sit above the Tier 0-3 leaves (mode arithmetic, Henderson chain, seasonal-MA,
// extreme-value). Unlike the leaves these read/write the X-11 COMMON state, so
// they take the live X13Context& (its x11opt / x11ptr / x11msc members carry the
// Fortran /optxin/, /optxdp/, /foscmn/, ... blocks) and thread ctx table fields
// into the leaves via .data(). ALL printing / WRITE / save is deferred.
#ifndef X13_X11_X11DRV_HPP
#define X13_X11_X11DRV_HPP

namespace x13 {

struct X13Context;

// vtc.f: VARIABLE TREND CYCLE. Applies a first-pass (Ny+1)-term Henderson to
// stci->stc, forms the irregular (stci/stc), measures the I-bar/C-bar ratio
// (Ratic, written to ctx.x11opt), and from it selects the final Henderson length
// (Nterm) and end-filter parameter (Tic) -- unless preselected via Ktcopt -- then
// re-applies the Henderson trend at that length. stc (trend, in/out) and stci
// (trend-cycle input) are 1-based PLEN buffers. Updates ctx.x11opt.{nterm,tic,
// ratic}. lsame short-circuits the symmetric pass when the length is unchanged.
void vtc(X13Context& ctx, double* stc, double* stci);

// sfmsr.f: MSR (moving seasonality ratio) global seasonal-filter selection
// (X-11-ARIMA/88). When Lterm==6 (auto) and Lmsr==6 (full run), iterates the
// global MSR over successively shorter whole-year spans (vsfa sets Ratis) to pick
// a 3x3 / 3x5 / 3x9 length, writing Lterm and the per-period Lter(1..Ny); the
// 3x5-throughout case sets work2.L3x5. A sliding-spans run (Lmsr!=6) instead
// reuses the whole-series decision (Lmsr). Always finishes with the vsfa (SI
// diagnostics) + vsfb (seasonal MA into sts) pass. Reads/writes ctx.x11opt
// (lterm/lmsr/lter/ny/ratis/muladd/ksect/rati) + ctx.x11msc (psuadd/shrtsf) +
// ctx.work2 (l3x5). The deferred print/save WRITEs (Lprt/Lsav) are not ported.
void sfmsr(X13Context& ctx, double* sts, double* stsi, int lfda, int llda,
           int lldaf);

// si.f: Part-B seasonal-from-SI driver. Runs the seasonal-MA pass (vsfa/vsfb)
// unless full-seasonal (Kfulsm==2), forms the irregular Sti (Stsi/Sts, or the
// pseudo-additive Stsi-Sts+1 / full-sum copy special cases), auto-selects the
// sigma limits (vtest/entsch when Ksect==1 & Ksdev<4), applies the extreme-value
// pass (xtrm), re-weights SI (replac), and re-derives the seasonal (vsfb). Reads/
// writes ctx.x11opt/x11srs/x11msc/xtrm/lzero. Series (Sts/Stsi/Sti) live on
// ctx.x11srs; the args mirror the Fortran call order for x11pt2 wiring. All
// table/punch output is deferred (dropped); Lfatal never trips here.
void si(X13Context& ctx, int ksect, int kfda, int klda, int nyr, int iforc,
        int nbcst, int kersa1, int ksdev1, int lfd1, int lld1, int kfulsm,
        int kfdax, int kldax);

}  // namespace x13

#endif  // X13_X11_X11DRV_HPP
