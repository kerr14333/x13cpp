// x11filt.hpp -- X-11 pure-numeric leaf routines (Tier 0 + Tier 1): the
// mode-arithmetic recombine primitives, the generic moving-average kernel, and
// the Henderson trend-filter chain (symmetric weights + Doherty 1993 asymmetric
// end filters). Direct ports of the vendored oracle Fortran, faithful to loop
// order and index arithmetic for bit-parity with the oracle binary.
//
// Index convention (matches core/src/numeric): pointers are 0-based C, but every
// Fortran 1-based index i accesses element [i-1]. Callers therefore pass a
// pointer whose element 0 is Fortran's X(1); range args (jfda/jlda, ib/ie, ...)
// stay in Fortran 1-based coordinates. Mode/flag COMMON-block state that the
// oracle reads implicitly (Muladd, Gudval, Tru7hn, ...) is passed explicitly
// here -- the X-11 ctx does not exist yet.
//
// ALL printing / WRITE is deferred (never ported). No table/save emission here.
#ifndef X13_X11_X11FILT_HPP
#define X13_X11_X11FILT_HPP

namespace x13 {

// Muladd adjustment mode (x11opt.cmn): 0=multiplicative, 1=additive, 2=logadd.
enum X11Mode { MODE_MULT = 0, MODE_ADD = 1, MODE_LOGADD = 2 };

// ---- Tier 0: mode-arithmetic recombine primitives --------------------------

// divsub.f: recombine by DIVIDE (muladd==0) or SUBTRACT (muladd!=0) over the
// 1-based inclusive range [jfda,jlda]. result[i] = a1[i]/a2[i] (mult) or
// a1[i]-a2[i] (add/logadd). Note the oracle only special-cases muladd==0, so
// logadd takes the subtract branch (it operates on already-logged series).
void divsub(double* result, const double* a1, const double* a2, int jfda,
            int jlda, int muladd);

// addmul.f: the inverse -- MULTIPLY (muladd==0) or ADD (muladd!=0) series x and
// y into z over [ib,ie]. z[i] = x[i]*y[i] (mult) or x[i]+y[i] (add/logadd).
void addmul(double* z, const double* x, const double* y, int ib, int ie,
            int muladd);

// logar.f: in-place natural log of x over the 1-based range [i,j].
void logar(double* x, int i, int j);

// antilg.f: in-place exp (antilog) of x over the 1-based range [i,j].
void antilg(double* x, int i, int j);

// setmv.f: reset the missing-value code. For i in [pos1ob,posfob], where
// mvind[i] is true, set srs[i]=mvval. (Prior adjustments can drag the missing
// sentinel off its code; this restores it.)
void setmv(double* srs, const bool* mvind, double mvval, int pos1ob,
           int posfob);

// change.f: month-to-month change of x into y over [ib,ie]. Additive
// (muladd==1) => plain difference y[i]=x[i]-x[i-1]. Otherwise => percent change
// (x[i]-x[i-1])/x[i-1] where the prior obs is "good" (gudval[i-1]), else DNOTST.
void change(const double* x, double* y, int ib, int ie, int muladd,
            const bool* gudval);

// divgud.f: divide a1 by a2 over [jfda,jlda] but only where gudval[i] is true;
// non-good positions get DNOTST (the multiplicative "good obs" divide).
void divgud(double* result, const double* a1, const double* a2, int jfda,
            int jlda, const bool* gudval);

// chkzro.f: clear the good-obs flag gudval[i] over [pos1,pos2] whenever any of
// the level series is non-positive. Requires ori>0 always; additionally (when
// kfulsm==0) sa>0 and ocal>0; and, gated on iyrt>0 / lrndsa, sa2>0 / sarnd>0.
// gudval is updated in place (only ever cleared, never set).
void chkzro(const double* ori, const double* sa, const double* sa2,
            const double* sarnd, const double* ocal, int pos1, int pos2,
            int kfulsm, int iyrt, bool lrndsa, bool* gudval);

// ---- Tier 0: moving-average / Henderson weight kernels ---------------------

// averag.f: apply an M-of-N moving average to x, storing into y over the valid
// centered range. ki=(M+N)/2-1; y[k] is written for k in [ib+ki, ie-ki] as the
// mean of the M overlapping N-sums, divided by M*N.
void averag(const double* x, double* y, int ib, int ie, int m, int n);

// hender.f: generate the HALF (symmetric) weights of an N-term Henderson moving
// average into w[0..m-1] where m=(N+1)/2; w[0] is the central weight. The full
// filter is w reflected. Weights sum to 1 and reproduce cubic trends.
void hender(double* w, int n);

// apply.f: apply symmetric half-weights w (length (N+1)/2) centered at 1-based
// index k of x. Returns w[0]*x[k] + sum_{i>=2} w[i]*(x[k-i+1]+x[k+i-1]).
double apply(const double* x, int k, const double* w, int n);

// ---- Tier 1: Henderson end filters (the classic X-11 divergence point) ------

// hndend.f: Doherty (1993) asymmetric end filter of length m for an nterm
// Henderson filter, given the half central weights w (from hender) and the
// end-filter parameter r (= 4/(Tic^2*pi)). Fills endwt[0..m-1]. Reproduces
// constant and linear trends for any r; r tunes the revision variance.
void hndend(int m, int nterm, const double* w, double* endwt, double r);

// ends.f: fill the l=(K-1)/2 trend points at each end of [ib,ie] using
// length-growing Henderson end filters (via hndend) with parameter rbeta. stci
// is the input series, stc the trend output (both 1-based); the interior is
// assumed already filled by the symmetric filter.
void ends(double* stc, const double* stci, int ib, int ie, int k, double rbeta);

// endsf.f: seasonal-MA asymmetric end weights (the 3x9 / 3x15 tables). For a
// single season's K values (simon, 1..K), fill the Nend end points at each side
// of savg from the flattened weight table w, normalized by the used weights;
// once the window would exceed K it falls back to the plain average (totals).
void endsf(const double* simon, double* savg, int k, const double* w, int nend);

// hndtrn.f: Henderson trend driver. Applies the symmetric N-term filter to the
// interior of stci->stc over [lfda,lldaf] (skipped when lsame), then, if lend,
// the Doherty end filters with rbeta=4/(tic^2*pi). The 7-term case reduces to a
// 5-term filter one point in at each end (unless tru7hn) and pins tic=0.001;
// tic is updated in place in that case.
void hndtrn(double* stc, const double* stci, int lfda, int lldaf, int nterm,
            double& tic, bool lend, bool lsame, bool tru7hn);

}  // namespace x13

#endif  // X13_X11_X11FILT_HPP
