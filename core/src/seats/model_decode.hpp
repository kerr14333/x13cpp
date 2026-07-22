// model_decode.hpp -- decode X-13's internal Mdl/Opr/Arimal/Arimap ARMA
// operator-list representation into the plain (p,d,q)(P,D,Q) orders and
// phi/bphi/th/bth coefficient arrays SEATS's own Fortran (translated from the
// standalone Bank-of-Spain program) expects. Direct port of two small oracle
// routines that sit between X-13's regARIMA model state and SEATS's entry
// point (SEATS/analts.f, which is otherwise mostly untouched vendored code
// operating on plain scalars/arrays, not Mdl/Opr):
//
//   nmlmdl.f (ansub9.f callsite ~1039) -- the actual decode: walks
//     Mdl(AR..MA)/Opr/Arimal/Arimap, buckets each operator into nonseasonal
//     or seasonal AR/MA by its Oprfac (period) versus 1 / Sp, verifies the
//     lag pattern has no gaps, then runs each group's raw -Arimap
//     coefficients through the TRANS0/TRANS2 bounded reparametrization the
//     real (pre-X-13) SEATS optimizer used for its search-space clamping.
//   transc.f (TRANS0 forward, TRANS2 inverse) -- the bounded transform
//     itself, for AR/MA operator groups of order 1-3 (SEATS's hard cap;
//     order >3 groups abend, matching nmlmdl.f's own Nn-exceeded check).
//
// PARITY NOTES:
//  - Classifying an operator by Oprfac(iopr) against 1 / model.sp is
//    numerically identical to the oracle's own classification, which
//    compares the operator's title STRING ('Nonseasonal AR', 'Seasonal AR',
//    ...) built by mkoprt.f purely from that same (period, Sp) pair
//    (mkoprt.f: Period==1 -> "Nonseasonal", Period==Sp -> "Seasonal", else
//    "Period N" and silently uncounted, mirrored here). Using Oprfac
//    directly avoids re-deriving the title string just to string-compare it
//    straight back out.
//  - TRANS0-then-TRANS2 round-trips to the identity for order-1 (trivial)
//    and order-2 (closed-form quadratic) groups, up to ordinary floating
//    rounding; for order-3 groups TRANS2 root-finds a cubic via
//    Newton-Raphson (5e-5 step tolerance, <=10 iterations) whose result
//    (Alph) the oracle computes but never actually uses in the P(=-C)
//    output it returns (dead code in transc.f -- verified by reading the
//    routine start-to-finish: the do-while's only exit is `goto 1000`, so
//    the `goto 1005` immediately after it is unreachable, and every path
//    reaches "P(M+I) = -C(I)" using only C(1..3), never Alph). This port
//    keeps the C(1..3)/P closed-form exactly but omits computing the
//    now-provably-unused Alph roots.
//  - None of the SEATS corpus specs (payems_seats MA(2); airline/unrate
//    (0,1,1)(0,1,1)) exercise an order-3 group, so the TRANS2 J==3 branch is
//    unexercised by the gate; it is ported for completeness (nmlmdl.f
//    unconditionally calls TRANS0/TRANS2 for any nonzero group order) but
//    not yet gated bit-exact against an oracle capture.
#ifndef X13_SEATS_MODEL_DECODE_HPP
#define X13_SEATS_MODEL_DECODE_HPP

#include "common/x13context.hpp"

namespace x13 {

// TRANS2 -- transc.f:4. Reconstruct model-space coefficients p(m+1..n) from
// search-space x(m+1..n) for an operator group of order n-m (1, 2, or 3).
// nn is the declared length of p/x (matches the Fortran DIMENSION bound).
void trans2(double* p, int nn, const double* x, int m, int n);

// TRANS0 -- transc.f:144. Forward transform: model-space p(ib..ie) ->
// bounded search-space x(ib..ie), clamping near the +-xl boundary (+-ur
// instead, for AR lags -- j<=iprs -- reflecting the stationarity-region
// convention). ur is always 1.0 at nmlmdl.f's callsites.
void trans0(const double* p, int nn, double* x, int ib, int ie, int iprs,
            double ur, double xl);

// Plain ARMA orders + coefficients decoded from ctx.model. phi/bphi/th/bth
// are 0-indexed, holding lag i+1's coefficient at index i (Fortran Phi(i) ->
// phi[i-1]); only the first p/bp/q/bq entries (whichever apply) are written.
struct SeatsModelOrders {
    int p = 0, bp = 0;  // AR orders: nonseasonal, seasonal
    int d = 0, bd = 0;  // differencing orders: nonseasonal, seasonal
    int q = 0, bq = 0;  // MA orders: nonseasonal, seasonal
    int mq = 0;         // seasonal period (== ctx.model.sp)
    int nfixed = 0;     // count of ARMA parameters held fixed by the user
    double phi[3] = {0.0, 0.0, 0.0};
    double bphi[3] = {0.0, 0.0, 0.0};
    double th[3] = {0.0, 0.0, 0.0};
    double bth[3] = {0.0, 0.0, 0.0};
    // RAW (pre-TRANS0) estimated MA coefficients. th/bth above are the
    // SEATS-transformed values (capped to the xl invertibility bound for the
    // canonical decomposition); th_raw/bth_raw are the untransformed regARIMA
    // estimates. The forecast/backcast EXTENSION of z (ansub1.f FCAST, run in
    // regARIMA before SEATS caps the model) must use these raw values, not the
    // capped ones -- otherwise a near-boundary seasonal MA (e.g. estimated
    // 0.978 snapped to 0.99) shifts the extension ~2e-5 and diffuses into
    // every s-table point. See estbur.cpp's fcast_extend call.
    double th_raw[3] = {0.0, 0.0, 0.0};
    double bth_raw[3] = {0.0, 0.0, 0.0};
};

// nmlmdl.f -- decode ctx.model's Mdl/Opr/Arimal/Arimap into `out`. xl is the
// seats{} admissible-region bound (the "Xl" argument nmlmdl.f's callsite
// passes; default 0.99, see ansub9.f SETDEFAULT / tools/seats_scope.md's
// GTSEAT note on real SEATS defaults living in ansub9.f, not gtinpt.f).
// Returns false (with ctx.error.lfatal set, mirroring the oracle's CALL
// abend) if any AR/MA operator group has more than 3 lags or a
// non-consecutive ("missing lag") lag pattern -- both are genuine SEATS
// limitations, not a porting shortcut.
bool seats_decode_model(X13Context& ctx, double xl, SeatsModelOrders& out);

}  // namespace x13

#endif  // X13_SEATS_MODEL_DECODE_HPP
