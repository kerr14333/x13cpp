// amdfct.hpp -- amdfct.f: the average absolute percentage forecast error
// ("aape"), the X-11-ARIMA forecast-quality diagnostic.
//
// For each of the last three years the model forecasts one year ahead from an
// origin that many years back, and the mean absolute percentage error of those
// twelve (or four) forecasts is reported; `aape.0` is the average of the three.
// 283 of the 331 `.udg` goldens carry it, and the port had none of it.
//
// SCOPE. Only the WITHIN-SAMPLE variant (`Outfct` false, the default) is
// ported -- the one every golden in this corpus uses. Out-of-sample re-fits
// the model three times over successively shorter spans and restores the whole
// estimation state around it (amdfct.f:70-90, :186-235, :270-300); it is
// walled, not silently approximated. Backcast error (`Bckcst`) likewise.
#ifndef X13_DIAG_AMDFCT_HPP
#define X13_DIAG_AMDFCT_HPP

namespace x13 {

struct X13Context;

struct AapeDiagnostics {
    // `false` reproduces the oracle's `aape.mode: none` -- either the guard at
    // amdfct.f:55-60 (not enough data before the three-year window to estimate
    // the model) or a non-positive variance at arima.f:872.
    bool ok = false;
    bool outofsample = false;   // walled; always false here
    double mape[4] = {0.0, 0.0, 0.0, 0.0};   // years 1,2,3 then their average
};

// arima.f:870-905 -- fill ctx.aape from the converged model. `trnsrs` is the
// clean transformed series over the span (the same buffer regvar was built on).
void aape_diagnostics(X13Context& ctx, const double* trnsrs);

}  // namespace x13

#endif  // X13_DIAG_AMDFCT_HPP
