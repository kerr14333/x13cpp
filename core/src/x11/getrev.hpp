// getrev.hpp -- getrev.f / putrev.f: the revisions-history CAPTURE, called from
// x11pt3 at five points during a history{} span replay (Irev==4).
//
// This is the counterpart of ssrit.f on the sliding-spans side: x11pt3 hands the
// finished component (seasonal factors, SA series, forced SA, rounded SA, trend)
// to getrev, which files this span's concurrent value into /revdta/ and -- on the
// LAST span only -- the whole final column. Every call site RETURNs from x11pt3
// straight afterwards unless a later component was also requested, so the oracle
// stops the pass at the last thing history{estimates=} asked for.
//
// The port used to skip all five (four walls plus one silently deferred site) and
// re-read ctx.x11srs AFTER x11pt3 returned. That is equivalent only while the
// buffer x11pt3 leaves behind is the one getrev was handed -- false under force{}
// (Stci2 / Stcirn) and false for the trend whenever an LS is folded back into the
// published D12 (stc2).
#ifndef X13_X11_GETREV_HPP
#define X13_X11_GETREV_HPP

namespace x13 {

struct X13Context;

// putrev.f -- file one observation of `inrev` into one revisions slot.
//   lrv    : this family was requested (Lrvsa / Lrvsf / Lrvtrn)
//   lrvch  : the CHANGE half was requested. IN/OUT and it is the live COMMON
//            flag: an additive run whose previous observation is non-positive
//            turns the whole percent-change history OFF for good (putrev.f:36-41).
//   rvdiff : IN/OUT, and getrev's LOCAL copy of Rvdiff -- the -1 written here is
//            what raises the "has ceased" warning; COMMON Rvdiff is untouched.
void putrev(const X13Context& ctx, const double* inrev, double& outrev,
            double& outch, double& outind, int iptr, bool lrv, bool& lrvch,
            int muladd, int itype, int& rvdiff, int indrev);

// getrev.f -- itype 0 = seasonal factors, 1 = seasonally adjusted, 2 = trend.
// `srs` is 1-based-by-Fortran-index (pass the raw buffer; the port indexes [i-1]).
void getrev(X13Context& ctx, const double* srs, int lstobs, int muladd,
            int itype, int ny, int iag, int iagr);

}  // namespace x13

#endif  // X13_X11_GETREV_HPP
