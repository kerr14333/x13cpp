// genqs.hpp -- the QS seasonality statistics (genqs.f / calcqs.f / calcqs2.f /
// qsdiff.f).
//
// The Pierce QS statistic tests for POSITIVE autocorrelation at the seasonal
// lag and its first harmonic. genqs computes it for six series (the original,
// the original adjusted for extreme values, the SA series and its EV twin, the
// irregular and its EV twin) over two spans; arima.f:1107 adds a seventh, the
// regARIMA residuals. Each pair is reported as (statistic, chi-square p-value).
#ifndef X13_DIAG_GENQS_HPP
#define X13_DIAG_GENQS_HPP

#include "gen/notset.hpp"

namespace x13 {

struct X13Context;

// calcqs.f: the QS statistic of z(iconce+1..nz) at lags mq and 2*mq. Both
// arguments are 1-based Fortran positions; `iconce` is the count of leading
// observations to SKIP, not the first index.
double calcqs(const double* z, int iconce, int nz, int mq);

// qsdiff.f: difference, mean-delete, then calcQS2 -- with calcQS2's PosCorr
// verdict optionally triggering one further difference. `srs` is 1-based.
void qs_diff(const double* srs, int pos1, int posf, bool lmodel, int nnsedf,
             int nseadf, int ny, double& qs);

// The 14 statistics genqs derives (the residual pair comes from run_pre_model).
// DNOTST marks "not computed", which is what suppresses a row.
struct QsStats {
    bool ran = false;      // genqs was reached
    bool lplog = false;    // `qslog`: some series was logged on the way in
    double qsori = prm::DNOTST, qsori2 = prm::DNOTST;
    // CB-25: the residual pair is initialised to ZERO, not to DNOTST, because
    // /arima/ QsRsd and QsRsd2 are only ever assigned inside `arima` itself
    // (arima.f:129-130). On a run with no regARIMA model that routine is never
    // called, so genqs reads the COMMON's static zero and reports it as a real
    // statistic -- which is why every no-model golden carries `qsrsd: 0.00000
    // 1.00000` while an identify-only run (Lmodel true, nothing estimated)
    // correctly omits the row. Reproduced verbatim: run_pre_model resets both
    // to DNOTST at the arima entry point, and nowhere else.
    double qsrsd = 0.0, qsrsd2 = 0.0;
    double qssadj = prm::DNOTST, qssadj2 = prm::DNOTST;
    double qsirr = prm::DNOTST, qsirr2 = prm::DNOTST;
    double qsoris = prm::DNOTST, qsoris2 = prm::DNOTST;
    double qssadjs = prm::DNOTST, qssadjs2 = prm::DNOTST;
    double qsirrs = prm::DNOTST, qsirrs2 = prm::DNOTST;

    // genqs.f:258-265 -- whether either block has anything to report at all.
    bool lqs() const;
    bool lqss() const;
};

// genqs.f, the direct (Iagr<4) path. Returns false on an unported branch.
bool genqs(X13Context& ctx, bool lseats);

// gennpsa.f -- the NP residual-seasonality VERDICT, a yes/no rather than a
// statistic: npsa.f differences the SA series, mean-deletes it, and compares
// Kendall's statistic against a fixed critical value (24.73 monthly / 11.35
// quarterly). Only the SA series and its extreme-value twin are tested, so the
// whole block is absent from a run that produces no adjustment.
struct NpStats {
    bool ran = false;
    bool lplog = false;             // `nplog`
    int npsadj = prm::NOTSET, npsadj2 = prm::NOTSET;
    int npsadjs = prm::NOTSET, npsadjs2 = prm::NOTSET;

    bool lnp() const;
    bool lnps() const;
};

// `iagr4` selects the INDIRECT pass (x11ari.f:367-370, after agr3): the same
// four verdicts over the aggregated buffers, written as `npindsadj` /
// `npindsadjevadj` / `npsindsadj` / `npsindsadjevadj` and kept in ctx.np_ind so
// the direct pass's own verdicts survive.
bool gennpsa(X13Context& ctx, bool lseats, bool iagr4 = false);

}  // namespace x13

#endif  // X13_DIAG_GENQS_HPP
