// automx.hpp -- automx.f: the CLASSIC X-11-ARIMA automatic model selection
// driven by a `pickmdl{}` spec. The sibling of automd.f (the TRAMO method), and
// a completely different algorithm: instead of identifying orders from the
// data, it ESTIMATES a fixed list of candidate models and keeps the best one
// that passes three screens.
//
//   for each candidate model (from `file=`, else the five built-in defaults):
//     estimate it (rgarma, plus outlier identification when `identify=all`)
//     score it   -- amdfct's three-year average forecast error `mape(4)`,
//                   the Ljung-Box p-value at lag 24 (12 quarterly),
//                   and the sum of each MA operator's coefficients
//     ACCEPT it if   mape(4) <= the running limit  (starts at `fcstlim`)
//                and Ljung-Box p*100 > `qlim`
//                and the NONSEASONAL MA sum < `overdiff`
//   `method=first` stops at the first acceptance; `method=best` keeps going and
//   the running limit tightens to each accepted model's own error, so the last
//   acceptance is the lowest-error one.
//
// The whole front was absent: `gt_pickmdl` routed all 11 arguments through
// gt_generic, so a pickmdl{} spec returned `OUTCOME: OK` having fitted NO ARIMA
// model at all (`nmodel: 0`, `nefobs: 144` against the oracle's `(0 1 2)(0 1 1)`
// and 131 on `extra/airline_pickmdl`).
//
// A candidate may carry a `*` (STAR) suffix, which marks it the DEFAULT model:
// if nothing is accepted, and the run still needs regARIMA preadjustment
// factors for some effect, the starred model is used anyway with forecasting
// switched off (`nofcst`, hvstar==2). `hvstar` tracks that: 0 none, 1 a star was
// seen, 2 the star is being used as the fallback, 3 a real acceptance
// superseded it.
//
// WALLED, not silently approximated (each fatals):
//   * automx's own AIC-regressor testing (`Itdtst`/`Leastr`/`Luser`/`Lomtst`
//     inside the candidate loop, automx.f:404-500 and :750-870). It re-runs
//     tdaic/easaic per candidate and interacts with the Picktd restore below.
//   * the Picktd trading-day restore (:255-292, :700-725) -- only reachable
//     once that AIC testing can change Picktd between candidates.
//   * `pickmdl{outofsample=yes}` (Outfer), rejected in the parser: the
//     out-of-sample half of amdfct is unported.
#ifndef X13_AUTOMDL_AUTOMX_HPP
#define X13_AUTOMDL_AUTOMX_HPP

#include "common/x13context.hpp"

namespace x13 {

// automx: run the pickmdl candidate search. `trnsrs` is the caller-owned
// transformed series (kept distinct from ctx.series.tsrs, which rgarma
// overwrites with residuals). On return the selected model is estimated in ctx
// and its designation is in ctx.arima.bstdsn / .nbstds.
//
//   hvmdl   out -- a model was ACCEPTED (false means every candidate failed a
//                  screen; with hvstar==2 the starred default is used anyway)
//   hvstar  in/out -- the default-model state described above; seed it 0
//   lsadj   in  -- Lx11.or.Lseats, passed through to nofcst's setxpt
//   lidotl  in  -- run outlier identification on each identified candidate
void automx(X13Context& ctx, double* trnsrs, int& frstry, int& nefobs,
            double* a, int& na, bool& hvmdl, int& hvstar, bool lsadj,
            bool lidotl);

}  // namespace x13
#endif  // X13_AUTOMDL_AUTOMX_HPP
