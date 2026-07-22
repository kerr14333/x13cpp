// seatsalloc.hpp -- SEATS AR-root allocation to canonical components. Direct
// port of oracle/fortran/sigsub.f:29-289 (F1RST + the isCloseTD helper).
//
// F1RST is the first step of the canonical decomposition proper (SIGEX calls
// it once, right after RPQ root-finds the model's full AR polynomial): each
// AR root (or conjugate pair) is convolved, via SEATS's own CONV (already
// ported, seatspoly.hpp), into one of six running denominator polynomials --
// nonstationary/stationary trend (chins/chis), nonstationary/stationary
// seasonal (psins/psis), or nonstationary/stationary cycle a.k.a. transitory
// (cycns/cycs) -- based on the root's modulus and argument relative to the
// seasonal frequency and the rmod/epsphi thresholds from seats{}. p==0 (no AR
// roots -- e.g. the pure-MA airline model) is a no-op.
//
// PARITY: the oracle's root order is complex pairs first, then real roots
// (see the "TALE CHECK" comment in the Fortran) -- F1RST relies on that order
// and only classifies the leading pair/root plus, for p==3, a single trailing
// real root at index 3. This mirrors the oracle exactly; it is not a general
// p-root classifier.
#ifndef X13_SEATS_SEATSALLOC_HPP
#define X13_SEATS_SEATSALLOC_HPP

namespace x13 {

// isCloseTD(w,mq) -- sigsub.f:257. True if a seasonal AR root's argument w
// (degrees) sits near a trading-day-related frequency for quarterly (mq==4)
// or monthly (mq==12) data. Always false for other mq (unevaluated in the
// oracle, which leaves IsCloseToTD at its caller-supplied value).
bool is_close_td(double w, int mq);

// F1RST -- sigsub.f:29. imz/rez/ar/modul are the root arrays from RPQ
// (0-based here, Fortran 1-based); p is the AR polynomial order (root
// count). cycns/psins/cycs/chins/chis/psis are running denominator
// polynomials (true-sign coefficients, constant term first) with their
// lengths (ncycns, npsins, ...) updated in place by CONV as roots are
// folded in -- caller must pre-initialize them (typically {1.0} length 1)
// per sigex.f:540-541. root0c/rootpic/is_close_to_td are set true on the
// branches the oracle sets them on; false unless already true and no
// matching branch fires (in/out, like the Fortran LOGICAL args). rootpis is
// accepted for signature parity but never assigned inside F1RST (the oracle
// never sets it here either -- see sigsub.f, no ROOTPIS= appears in the
// body).
void f1rst(int p, const double* imz, const double* rez, const double* ar,
           double epsphi, int mq, double* cycns, int& ncycns, double* psins,
           int& npsins, double* cycs, int& ncycs, double* chins, int& nchins,
           double* chis, int& nchis, const double* modul, double* psis,
           int& npsis, double rmod, bool& root0c, bool& rootpic,
           bool& rootpis, bool& is_close_to_td);

}  // namespace x13

#endif  // X13_SEATS_SEATSALLOC_HPP
