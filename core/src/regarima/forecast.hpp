// forecast.hpp -- fcstxy.f: minimum-mean-square-error forecasts of the
// transformed, regression-adjusted regARIMA series, with their standard errors.
// Sits above the exact ARMA filter (armafl) and the estimation output (Var, B,
// Chlxpx). Prints/table output are handled elsewhere; this is the pure numeric
// forecast engine.
#ifndef X13_REGARIMA_FORECAST_HPP
#define X13_REGARIMA_FORECAST_HPP

#include "common/x13context.hpp"

namespace x13 {

// fcstxy.f: compute nfcst-ahead forecasts of the (Box-Cox transformed, prior-
// and regression-adjusted) series and their forecast standard errors. fctori is
// the length of the undifferenced series (Xy holds fctori rows of the [X:y]
// design). The forecast recursion runs the estimated full AR*differencing and MA
// operators (built by polyml over the model operators) forward over the forecast
// span, seeded by the exact ARMA-filtered residuals (armafl); resid then applies
// the regression adjustment Ay-(AX-X_f)b to give the forecasts. Standard errors
// come from the psi(B)=MA(B)/AR(B) weights (ratpos) plus, for the unfixed
// regressors, the design-uncertainty term X_f(X'X)^-1 X_f' via the packed
// Cholesky Chlxpx (dppsl/yprmy). Outputs (all length nfcst): fcst (forecasts on
// the transformed scale, mean included), se (forecast standard errors), rgvar
// (the regression-variance contribution rgvar(i)=Var*X_f(X'X)^-1 X_f').
void fcstxy(X13Context& ctx, int fctori, int nfcst, double* fcst, double* se,
            double* rgvar);

// fcstout: the numeric core of prtfct.f's forecast-output (LFOROS) path. Runs
// fcstxy, maps the transformed forecast back to the original scale (invfcn, or
// the lognormal mean-correction lgnrmc when lognrm && lam==0), and forms the
// two-tailed confidence band [invfcn(fcst - cv*se), invfcn(fcst + cv*se)] with
// critical value cv = dinvnr((ciprob+1)/2). Predefined length-of-period/leap-year
// prior factors (Priadj>1) are reapplied over the forecast window after inverse
// transformation, matching prtfct.f's original-scale output. Reads the estimated
// model + forecast options off ctx (fcntyp/lam/ciprob/lognrm), stores the result
// on ctx.forecasts (no file output). fctdrp is the number of retained forecast-
// period observations dropped from the fit.
void fcstout(X13Context& ctx, int nfcst, int fctdrp, double ciprob, bool lognrm);

}  // namespace x13

#endif  // X13_REGARIMA_FORECAST_HPP
