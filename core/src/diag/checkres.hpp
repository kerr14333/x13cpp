// checkres.hpp -- the check{} spec's residual diagnostics (arima.f:1044-1102).
//
// Everything here runs on the final regARIMA residuals, after the model has
// converged, and is DIAGNOSTIC ONLY: nothing downstream reads it. That is why
// the whole front went missing without any gate noticing -- `gt_check` routes
// check{}'s arguments through `gt_generic`, i.e. parses and drops them.
//
// It is nonetheless real output. Every one of these statistics is a savelog
// canary the oracle writes to the `.udg` on ANY run with a model, whether or
// not the spec asks for check{}: editor.f:909 sets Mxcklg = 2*Sp when
// `Lsumm > 0` (the `-s` flag every golden in this corpus was blessed with) and
// Mxcklg is still 0. 287 of the 331 `.udg` goldens carry the block.
//
// Ported: acf.f (sample ACF + Bartlett standard errors + the Ljung-Box and
// Box-Pierce Q sequences), pacf.f (Yule-Walker partial ACF), acfdgn.f (which
// of those lags are SIGNIFICANT, at `acflimit`/`qlimit`), nrmtst.f (skewness,
// Geary's a, kurtosis, each against a one-percent point interpolated from its
// own table -- intrpp.f + nrmtst.var), the Durbin-Watson statistic inlined at
// arima.f:1075-1090, and the Friedman/Kendall seasonality test
// (ansub11.f:1303's `kendalls` + chisq) at arima.f:1092-1101.
#ifndef X13_DIAG_CHECKRES_HPP
#define X13_DIAG_CHECKRES_HPP

#include <vector>

namespace x13 {

struct X13Context;

// One flagged lag, shared by the Q and the ACF/PACF significance lists.
struct CheckLag {
    int lag = 0;
    double a = 0.0;    // Q, or the sample (p)acf
    double b = 0.0;    // p-value, or the standard error
    int df = 0;        // Q only
    double t = 0.0;    // (p)acf only
};

struct CheckDiagnostics {
    bool ran = false;

    // acfdgn.f's two thresholds (gtinpt.f:295-296). NOTE Qcheck's default is
    // PT5 = 0.05 in gtinpt.f:21 -- the same misleading parameter name that
    // already bit the x11regression Cvxalf default; it is a probability, not
    // a half.
    double qlimit = 0.05;
    double acflimit = 1.6;
    int mxlag = 0;      // the resolved Mxcklg
    int nlagbl = 0;     // min(Mxcklg, 2*Sp) -- the window the counts run over

    std::vector<CheckLag> lbq;      // Ljung-Box    (Iqtype 0)
    std::vector<CheckLag> bpq;      // Box-Pierce   (Iqtype 1)
    std::vector<CheckLag> sigacf;
    std::vector<CheckLag> sigpacf;

    // nrmtst.f. The flags carry the oracle's own significance MARKERS, which
    // are part of the savelog line: skewness gets '-'/'+'/' ', Geary's a and
    // kurtosis get '*'/' '.
    bool have_skew = false, have_geary = false, have_kurt = false;
    double skewness = 0.0, geary = 0.0, kurtosis = 0.0;
    char skew_mark = ' ', geary_mark = ' ', kurt_mark = ' ';

    bool have_dw = false;
    double dw = 0.0;

    bool have_friedman = false;
    double friedman = 0.0;      // Kendall's statistic
    int friedman_df = 0;
    double friedman_pv = 0.0;
};

// acf.f -- sample autocorrelations r(1..nr) with Bartlett standard errors
// se(1..nr), plus the Q sequence (qs/dgf/qpv, the oracle's /autoq/ COMMON).
// `iqtype` 0 = Ljung-Box (the (n+2)/(n-i) weighting), 1 = Box-Pierce.
// Returns false when the series has no variance (C0 <= 0), which is the
// oracle's "Can't calculate an ACF for a model with no variance" note.
bool check_acf(const double* z, int nz, int nefobs, double* r, double* se,
               int nr, int np, int sp, int iqtype, bool lmu,
               double* qs, int* dgf, double* qpv);

// pacf.f -- the Yule-Walker partial ACF. Reads r (the sample ACF) and
// OVERWRITES it with the pacf, exactly as the Fortran does (`copy(fkk,Nr,1,R)`).
void check_pacf(int nefobs, double* r, double* se, int nr);

// ansub11.f:1303 -- Kendall's statistic behind the Friedman seasonality test:
// rank each whole year's `mq` observations (average ranks on ties, the leading
// partial year DROPPED), sum the ranks by period, and form
// 12*SS/((mq+1)*mq*ny). Shared with npsa.f (the NP residual-seasonality test).
double kendalls(const double* x, int nz, int mq);

// arima.f:1044-1102 -- run the whole block on the residuals `a(1..na)` and
// record it on ctx.check. No-op unless the model converged with a variance.
void check_residuals(X13Context& ctx, const double* a, int na, int nefobs);

// pracf2.f:37-60 -- the two NOTEs the oracle writes INSTEAD of the
// squared-residual ACF. The table itself is deferred print surface.
void pracf2_notes(X13Context& ctx, int nefobs);

}  // namespace x13

#endif  // X13_DIAG_CHECKRES_HPP
