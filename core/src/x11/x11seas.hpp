// x11seas.hpp -- X-11 Tier 2 seasonal moving-average chain. Direct ports of the
// vendored oracle Fortran seasonal-factor routines: vsfa.f (preliminary
// seasonal MA pass + I/S ratios), vsfb.f (the 3x3 / 3x5 / 3x9 / 3x15 / stable /
// 3-term seasonal MA selector), and vsfc.f (the centering 2xNyr MA over the
// seasonals). Faithful to loop order and index arithmetic for bit-parity with
// the oracle binary.
//
// Index convention (matches x11filt): pointers are 0-based C, but every Fortran
// 1-based index i accesses element [i-1]; span/range args (lfda/llda, nyr, ...)
// stay in Fortran 1-based coordinates. COMMON-block state that the oracle reads
// implicitly (Muladd, Psuadd, Lterm, Lter, Ksect, Shrtsf, Rati/Ratis, and the
// /work/ scratch Temp) is passed explicitly here -- the X-11 ctx does not exist
// yet. ALL printing / WRITE is deferred.
#ifndef X13_X11_X11SEAS_HPP
#define X13_X11_X11SEAS_HPP

namespace x13 {

// fis.f: the I/S-ratio "number of years" adjustment factor. Returns the length
// correction f(n) for a series of n complete years and, through cs, the paired
// SBAR correction. For n<6 both come from an 8-entry lookup; for n>=6 they use
// the closed-form asymptotic expressions. Exposed for unit testing.
double fis(double& cs, int n);

// vsfc.f: center the seasonal factors sts by dividing them by a 2xNyr moving
// average (mode divsub). The MA is formed into caller scratch temp (>= same
// length as sts), the k=Nyr/2 missing end terms are filled by repeating either
// the same-period MA value (when that period's filter Lter==5, i.e. 3x15) or
// the nearest available MA value, then sts <- sts / temp over [lfda,llda].
// lter is the per-period filter vector (Fortran 1-based Lter(1..Nyr)); muladd
// selects the divsub branch (0=divide, else subtract).
void vsfc(double* sts, int lfda, int llda, int nyr, const int* lter,
          double* temp, int muladd);

// vsfa.f: preliminary seasonal-factor pass. For each of the Nyr calendar
// periods it lays the SI ratios of that period into a 7-term-MA estimate of the
// seasonal, forms an estimate of the irregular, and accumulates the I-bar /
// S-bar ratios into rati (Fortran Rati(1..3*Nyr): [1..Nyr]=I ratios,
// [Nyr+1..2*Nyr]=S ratios, [2*Nyr+1..3*Nyr]=I/S), plus the global MSR ratis.
// muladd/psuadd select the mode branches. rati must hold 3*Nyr doubles
// (0-based rati[idx-1]); ratis is written.
void vsfa(const double* stsi, int lfda, int llda, int nyr, int muladd,
          bool psuadd, double* rati, double& ratis);

// vsfb.f: apply the selected seasonal moving average to the SI ratios stsi,
// storing the smoothed seasonal factors into sts, then center them via vsfc.
// The per-period filter is chosen from lterm / lter (Mtype = filter+1: 2=3x3,
// 3=3x5, 4=3x9, 5=3x15, 6=stable, 7=3-term), with the short-series and
// preselection fallbacks of the oracle. ksect (==1 selects a 3x3 when a
// preselected/AllStable filter is requested) and shrtsf (short-series seasonal
// filter honoring) are the gating flags. temp is vsfc scratch; muladd selects
// the recombine mode.
// pmtype (optional) receives the final Mtype (seasonal moving-average code) set
// by the last filter block -- the oracle keeps it in the x11opt common where
// shrink() later reads it (vsfb.f:51/68). Pass &ctx.x11opt.mtype at the final
// (D-pass) seasonal call so shrink sees the same value.
void vsfb(double* sts, const double* stsi, int lfda, int llda, int nyr,
          int lterm, const int* lter, int ksect, bool shrtsf, double* temp,
          int muladd, int* pmtype = nullptr);

}  // namespace x13

#endif  // X13_X11_X11SEAS_HPP
