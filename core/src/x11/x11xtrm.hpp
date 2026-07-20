// x11xtrm.hpp -- X-11 Tier 3 extreme-value adjustment leaves. Direct ports of
// the vendored oracle Fortran: xtrm.f (sigma-limit extreme detection driver),
// sdxtrm.f (five-year sigma / MAD computation), wtxtrm.f (graduated extreme
// weight), replac.f (replace extremes with a reweighted moving average),
// weight.f (preliminary 2xNyr / 2x4 trend-cycle weight application), vtest.f +
// entsch.f (Cochran heteroskedasticity test + Ksdev auto-select), and trbias.f
// (log-additive trend bias correction). Faithful to loop order, index
// arithmetic, and the sigma-boundary comparisons for bit-parity with the oracle.
//
// Index convention (matches x11filt): 0-based C pointers, Fortran index i ->
// element [i-1]; span args stay Fortran 1-based. COMMON-block state (Ny, Muladd,
// Ksdev, Imad, Sigmu/Sigml, Lsp, Csigvc, Tru7hn, and the Stwt/Stdper/Stdev
// arrays) is passed explicitly -- the X-11 ctx does not exist yet. ALL WRITE
// (the Stdev weight-table printing) is deferred; Stdev is still computed.
#ifndef X13_X11_X11XTRM_HPP
#define X13_X11_X11XTRM_HPP

namespace x13 {

// rho2.f: Tukey-biweight rho influence function used by the MAD tau adjustment
// (Imad>=3). Saturates to 6.502 for |u|>2.798. Exposed for unit testing.
double rho2(double u);

// wtxtrm.f: graduated extreme weight for one irregular value. temp=|x-xbar|/
// stddev; weight is lstwt inside [.,sigml], linearly graded to 0 across
// (sigml,sigmu] (istep!=1 only), 0 above sigmu on the first pass, and a
// temporary -1 (later zeroed) above sigmu on the second pass. Returns the new
// weight.
double wtxtrm(double x, double xbar, double stddev, double sigmu, double sigml,
              int istep, double lstwt);

// sdxtrm.f: five-year standard deviation (imad==0) or median-absolute-deviation
// standard error (imad>=1) of the irregulars xi over the strided range [l,m]
// step nsp, omitting second-pass extremes (stwt==0). imad selects: 0=RMS,
// 1/3=MAD of |xi-xbar|, 2/4=MAD of |log xi| with the log-normal rescale, 3/4
// add the rho2 tau correction. lgrp / csigvc / ksdev gate the calendarsigma
// grouping. Uses the numeric shlsrt.
double sdxtrm(const double* xi, double xbar, int l, int m, int nsp, int imad,
              int istep, int ny, bool lgrp, const double* stwt,
              const bool* csigvc, int ksdev);

// xtrm.f: compute irregular-component weights and flag extremes. Runs two
// passes (istep 1,2); the branch is chosen by ksdev (4=grouped calendarsigma,
// >0=per-period heteroskedastic, 0=standard five-year X-11). stwt[1..klda] is
// initialized to 1 and updated in place; stdper[1..ny] and stdev[1..PYRS+1] are
// computed (stdev feeds the deferred weight table). kfda/klda bracket the span
// interior, kfdax/kldax the full (forecast-extended) span.
void xtrm(const double* xi, int kfda, int klda, int kfdax, int kldax, int ny,
          int muladd, int ksdev, int imad, double sigmu, double sigml, int lsp,
          double* stwt, double* stdper, double* stdev, const bool* csigvc);

// replac.f: replace every value of x whose weight stwt is < 1 by a weighted
// average of the flagged value (times its weight) and the four nearest
// full-weight values, storing the replacement into y as well. nm is the
// seasonal stride (1 = whole-series irregular); for nm>1 a period with fewer
// than four full-weight values falls back to that period's SI average. x is
// modified in place. Reuses the numeric totals().
void replac(double* x, double* y, const double* stwt, int lfda, int llda,
            int nm);

// weight.f: preliminary trend-cycle estimate via the centered 24-term (monthly,
// mq!=2) or 8-term (quarterly, mq==2) moving average of a into b over [i1,i2],
// with the published Census end-weight tables applied to the six (two) points
// at each end. b is zeroed over [i1,i2] first.
void weight(const double* a, double* b, int i1, int i2, int mq);

// vtest.f: Cochran's test for heteroskedastic irregulars over the periods of x
// in [ib,ie]. Sets i1=1 when the largest period variance share exceeds the
// tabulated critical value (separate tables for monthly and quarterly, indexed
// by the minimum per-period count, capped at 40), else 0.
void vtest(const double* x, int& i1, int ib, int ie, int ny, int muladd);

// entsch.f: Ksdev auto-selection helper. From the (ken,ker) sigma-limit state
// and the mode indicator iv it fills (ken1,ker1) via the 5-way branch on
// ken+ker+1.
void entsch(int ken, int ker, int& ken1, int& ker1, int iv);

// trbias.f: log-additive trend bias correction (Thompson & Ozaki 1992). Forms
// sig=exp(sum sti^2 / (2*(L2-L1+1))), smooths the seasonals sts with a
// (2*ny-1)-term Henderson filter (tic=4.5), and scales stc by biasfc=sig*trend
// over [l1,l2]. tru7hn threads into the Henderson end chain.
void trbias(double* stc, const double* sts, const double* sti, int l1, int l2,
            double* biasfc, int ny, bool tru7hn);

}  // namespace x13

#endif  // X13_X11_X11XTRM_HPP
