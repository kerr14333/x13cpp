// armafilt.hpp -- ARMA filter routines that operate on the model-operator
// structure (arimap coefficients, arimal lags, opr operator table), as opposed
// to the plain-coefficient-array kernels in numeric.hpp (uconv/xpand/euclid).
// Direct ports of the vendored oracle Fortran; pointers are Fortran-base
// (arimap[0]==Arimap(1), arimal[0]==Arimal(1), opr[0]==Opr(0)).
#ifndef X13_REGARIMA_ARMAFILT_HPP
#define X13_REGARIMA_ARMAFILT_HPP

namespace x13 {

// arflt.f: conditional (truncated) AR/differencing filter applied in place to
// the series in c. For each operator [begopr..endopr] it shortens the series by
// that operator's max lag (via maxlag) and convolves. The read of c[off] and
// c[off-lag] ABOVE the write cursor c[i] is load-bearing: strictly ascending i
// keeps those entries at their pre-filter values. neltc returns the final
// length. nelta is the input length.
void arflt(int nelta, const double* arimap, const int* arimal, const int* opr,
           int begopr, int endopr, double* c, int& neltc);

// mltpos.f: multiply the series c by the Difference/AR/MA operators
// [begopr..endopr], producing neltc output elements. Uses a work buffer (only
// indices 1..neltc are ever touched, so a neltc-sized buffer is faithful to the
// fixed-PXA Fortran) and copies work->c after each operator. The secpas flag is
// load-bearing: the FIRST operator pass zero-pads beyond the input length nelta
// (tmp=0 and the c[itmp] term is gated by itmp<=nelta); every subsequent pass
// (secpas true) treats the full neltc-length series with no nelta gate.
void mltpos(int nelta, const double* arimap, const int* arimal, const int* opr,
            int begopr, int endopr, int neltc, double* c);

// chkrts.f: LOGICAL detector -- checks whether the roots of each operator's
// theta(B)=0 lie outside the unit circle (invertible). Despite the "makes them
// invertible" comment it does NOT write back to arimap: the reflection
// (Levinson-Durbin) recursion runs on a LOCAL coef buffer purely as an
// invertibility test. Returns true if ANY operator in [begopr..endopr] is
// non-invertible, and sets prbfac to the (last) offending operator index; an
// operator whose every lag is fixed (arimaf all true) is skipped. oprfac holds
// each operator's seasonal factor (lag/factor gives the polynomial degree).
bool chkrts(const double* arimap, const int* arimal, const bool* arimaf,
            const int* opr, const int* oprfac, int begopr, int endopr,
            int& prbfac);

}  // namespace x13

#endif  // X13_REGARIMA_ARMAFILT_HPP
