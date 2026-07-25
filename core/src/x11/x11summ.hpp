// x11summ.hpp -- the X-11 PART-F summary measures and quality statistics:
// sumry.f, vars.f/varlog.f/varian.f, avedur.f, issame.f, isfals.f and f3cal.f,
// plus the Part-F body of x11pt4.f that drives them.
//
// This is the diagnostics half of x11pt4; the E tables (E1-E8/E11/E18) it also
// emits are pure print/save and stay deferred. Everything here writes into the
// /inpt2/, /work2/ and /optxin/ Mcd COMMONs, which are what svf2f3.f then prints
// as the .udg `f2.*` / `f3.*` savelog block.
//
// Index convention as elsewhere in core/src/x11: 0-based C pointers, Fortran
// index i -> element [i-1]; range args stay Fortran 1-based.
#ifndef X13_X11_X11SUMM_HPP
#define X13_X11_X11SUMM_HPP

namespace x13 {

struct X13Context;

// sumry.f: per-span (lag k = 1..Ny) summary measures of X over [i,j], skipping
// observations for which gudval is false. Changes are percent changes when
// muladd==0, plain differences otherwise.
//   xbar  = mean |change|                 (always written)
//   xbar2 = mean signed change            (iopt 0 or 1)
//   xsq   = xbar^2                        (iopt 0 or 2)
//   xsd   = s.d. of the signed changes    (iopt 0 or 1)
// A span with no good pairs yields DNOTST. iopt 3 writes xbar only, so callers
// legitimately pass scratch for the outputs their iopt does not touch.
void sumry(const double* x, double* xbar, double* xbar2, double* xsq,
           double* xsd, int iopt, int i, int j, int ny, int muladd,
           const bool* gudval);

// varian.f: sum of squared deviations of x over [i,j]. iopt 0 = about the
// sample mean, 1 = about zero, 2 = about one. NOT divided by n.
double varian(const double* x, int i, int j, int iopt);

// varlog.f: the same on log(x), over good and strictly-positive observations
// only. iopt==1 assumes a log mean of zero. Returns DNOTST when iopt!=1 and no
// observation qualifies (so the mean cannot be formed).
double varlog(const double* x, int i, int j, int iopt, const bool* gudval);

// vars.f: varlog for multiplicative/log-additive modes, varian for additive.
double vars(const double* x, int i, int j, int iopt, int muladd,
            const bool* gudval);

// avedur.f: average duration of run -- (M-L) divided by the number of monotone
// runs in y over [L,M]. Ties at the START of the series are skipped without
// counting a run; ties later on do not break a run either.
void avedur(const double* y, int l, int m, double& adr);

// issame.f: are all GOOD values of a 1-based array over [l1,l2] equal to the
// value at l1? (Note l1 itself is compared regardless of its own good flag.)
bool issame(const double* lsrs, int l1, int l2, const bool* gudval);

// isfals.f: is at least one element of a logical array over [l1,l2] FALSE?
bool isfals(const bool* lsrs, int l1, int l2);

// f3cal.f: the M1-M11 quality statistics, the composite Q (Qual), Q without M2
// (Q2m2) and the count of M statistics at or above 1 (Kfail). Reads the summary
// measures x11pt4_partf has just written plus /tests/ Test1,Test2 (from combft),
// Ratic/Ratis and Mcd; writes /work2/ Qu,Qual,Q2m2,Nn,Nyrs,Kfail.
void f3cal(X13Context& ctx, const double* sts, int& ifail);

// x11pt4.f from "PART F" (:320) to the f3cal call (:713) -- the summary-measure
// battery: Pbar/Psq/Vp, Tdbar/Tdsq/Vtd, Ibar/Isq/Isd/Adri, Ombar, Imbar/Vi,
// Sbar/Ssq/Vs, Cbar/Csq/Adrc/Vc, Obar/Osq, the Osq2 normalisation, Smic, the
// Cibar/Cisd/Adrci and Cimbar measures, Mcd + the MCD moving average
// (Smbar/Smsd/Adrmcd), the irregular autocorrelations Autoc, and then f3cal.
//
// Returns false when the oracle's "diagnostics cannot be generated" guard fires
// (a zero/uncomputable variance, x11pt4.f:546 `IF(lsame)RETURN`); the caller
// must then leave the f2/f3 block unemitted, exactly as the oracle does.
//
// WORKS ON COPIES of Series/Stci/Stcime/Stome/Stc/Sti. The oracle mutates those
// buffers in place here (a log/antilog round trip on Stc, divide-then-multiply
// round trips on the rest) -- but it has already punched d10-d16 by the time
// x11pt4 runs, so its saved tables are the PRE-x11pt4 values. The C++ harness
// dumps at exit instead, so the round-trip perturbation would otherwise leak
// into d11/d12/d16 at ~1 ulp. Copying is the faithful choice for the observable
// output. `sti_int` / `stc_int` are the INTERNAL (pre-publication) D13 / D12,
// i.e. before x11pt3 folds the AO/TC outliers back into D13 and the LS into D12.
bool x11pt4_partf(X13Context& ctx, const double* sti_int, const double* stc_int);

}  // namespace x13

#endif  // X13_X11_X11SUMM_HPP
