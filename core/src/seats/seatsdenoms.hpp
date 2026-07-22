// seatsdenoms.hpp -- SEATS nonseasonal/seasonal denominator initialization
// from the model's differencing orders and (optional) near-unit seasonal-AR
// root. Direct port of the inline setup in oracle/fortran/sigex.f:476-540,
// the code SIGEX runs immediately BEFORE its one F1RST call (seatsalloc.hpp)
// to seed chins/chis/psins/psis/cycns/cycs with the nonstationarity that
// comes from differencing (d, bd) -- as opposed to F1RST's job, which folds
// in the *stationary AR roots'* modulus/frequency classification on top of
// this base.
//
// PARITY: the (1-B)^dplusd expansion (Chins) is ported as the oracle's own
// in-place Pascal's-triangle-style update loop, NOT rebuilt via repeated CONV
// calls against a (1,-1) factor -- mathematically equivalent, but the
// project's parity strategy is structural correspondence to the Fortran, not
// just numerical equivalence.
//
// The `bphi` near-unit-seasonal-AR-root heuristic only looks at bphi(mq+1)
// (0-based here: bphi[mq]) -- the oracle's own comment context implies this
// assumes a length-1 seasonal AR factor (bp<=1); it is ported verbatim
// (including the asymmetric >0/<0 branches for Cycs vs the Chins/Psins fold)
// rather than generalized, since that is exactly what the oracle does.
#ifndef X13_SEATS_SEATSDENOMS_HPP
#define X13_SEATS_SEATSDENOMS_HPP

namespace x13 {

// seats_init_denoms -- sigex.f:476-540. d/bd are the regular/seasonal
// differencing orders (Nfcst-independent model orders); bp is the seasonal
// AR order (0 or, per the oracle's own restriction here, effectively <=1);
// bphi is the seasonal AR polynomial as a length-(mq+1) true-sign coefficient
// array (bphi[0]==1, only bphi[mq] populated for bp==1 -- see the .hpp
// comment above); mq is the seasonal period. All six output denominators
// must be pre-sized for at least: chins mq*2+2, chis 6, psins 27, psis 16,
// cycns 2, cycs mq+2 (mirrors the Fortran's Chins(64)/Chis(5)-class fixed
// dimensions with headroom for the two CONV folds).
void seats_init_denoms(int d, int bd, int bp, const double* bphi, int mq,
                        double* chins, int& nchins, double* chis, int& nchis,
                        double* psins, int& npsins, double* psis, int& npsis,
                        double* cycns, int& ncycns, double* cycs, int& ncycs);

}  // namespace x13

#endif  // X13_SEATS_SEATSDENOMS_HPP
