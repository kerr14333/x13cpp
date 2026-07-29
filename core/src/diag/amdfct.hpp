// amdfct.hpp -- amdfct.f: the average absolute percentage forecast error
// ("aape"), the X-11-ARIMA forecast-quality diagnostic.
//
// For each of the last three years the model forecasts one year ahead from an
// origin that many years back, and the mean absolute percentage error of those
// twelve (or four) forecasts is reported; `aape.0` is the average of the three.
// 283 of the 331 `.udg` goldens carry it, and the port had none of it.
//
// SCOPE. Both the WITHIN-SAMPLE variant (`Outfct`/`Outfer` false, the default)
// and the OUT-OF-SAMPLE one are ported. Out-of-sample re-fits the model three
// times over successively shorter model spans, forecasting each time from the
// new span's end, and restores the whole estimation state afterwards
// (amdfct.f:70-90, :186-235, :270-300). Backcast error (`Bckcst`, reached only
// from `pickmdl{bcstlim=}` at automx.f:906) is still unported.
#ifndef X13_DIAG_AMDFCT_HPP
#define X13_DIAG_AMDFCT_HPP

namespace x13 {

struct X13Context;

struct AapeDiagnostics {
    // `false` reproduces the oracle's `aape.mode: none` -- either the guard at
    // amdfct.f:55-60 (not enough data before the three-year window to estimate
    // the model) or a non-positive variance at arima.f:872.
    bool ok = false;
    bool outofsample = false;   // the `aape.mode:` the .udg reports
    double mape[4] = {0.0, 0.0, 0.0, 0.0};   // years 1,2,3 then their average
};

// arima.f:870-905 -- fill ctx.aape from the converged model. `trnsrs` is the
// clean transformed series over the span (the same buffer regvar was built on).
//
// `lauto` is amdfct.f's `Lauto`: it selects `Outfer` over `Outfct` for the
// out-of-sample switch and, on the out-of-sample path, is CLEARED when a
// re-estimation fails so an automatic-model caller can drop the candidate
// instead of abending. Pass null on the ordinary (non-automatic) path.
void aape_diagnostics(X13Context& ctx, const double* trnsrs,
                      bool* lauto = nullptr);

}  // namespace x13

#endif  // X13_DIAG_AMDFCT_HPP
