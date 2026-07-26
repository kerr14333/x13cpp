// rev_outlier.hpp -- rmotrv.f / chkorv.f: the outlier regressors a revisions
// history has to HOLD BACK.
//
// A history{} span that ends at date T must not know about an outlier dated
// after T -- the whole point of the analysis is what the adjustment looked like
// with only the data available then. So before the span loop starts, revdrv.f
// deletes every outlier regressor dated after the FIRST revision date from the
// design and files it away (`rmotrv`), and each span re-introduces the ones its
// own model span now covers (`chkorv`).
//
// **Neither is behind a flag.** `rmotrv` is called unconditionally at
// revdrv.f:302 and `chkorv` at revdrv.f:589 for every history{} run that has a
// regARIMA model, whatever `outlier=` says; the flag only decides whether the
// deleted outliers are SAVED for re-introduction (`lotlrv`) or simply dropped.
// Without this the engine fits every span with the full-series outlier set --
// measured on airline + `regression{variables=(ao1957.jan ls1958.jul)}` +
// `history{start=1955.jan}`: sar 9.8e-1 / sae 9.1e-3 against tolerances of
// 5e-3 / 1e-5. An `outlier{}` spec that actually identifies something is wrong
// the same way (sar 9.2e-1), because the automatic outliers land in the design
// as ordinary regressors and rmotrv treats them alike.
#ifndef X13_DRIVER_REV_OUTLIER_HPP
#define X13_DRIVER_REV_OUTLIER_HPP

#include <string>
#include <vector>

namespace x13 {

struct X13Context;

// The Otrttl/Otrptr/Botr/Fixotr store: the outliers rmotrv took out, in the
// order it took them (it walks the design backwards, so this is descending
// column order -- which is the order chkorv re-adds them in).
struct RevOtlStore {
    std::vector<std::string> ttl;   // Otrttl -- the column title, e.g. "AO1957.Jan"
    std::vector<double> b;          // Botr   -- the main run's coefficient
    std::vector<char> fix;          // Fixotr -- Regfx of that column
    bool empty() const { return ttl.empty(); }
    int size() const { return static_cast<int>(ttl.size()); }
};

// rmatot.f -- history{outlier=remove}: strike every AUTOMATICALLY identified
// outlier (the ones an `outlier{}` spec found on the main run) from the design,
// so no span inherits a find it could not have made itself. Only the delete
// path is ported; `outlier=auto`'s save-and-re-enter arm needs each span to run
// the identification again, which this driver does not do (run_history fatals).
void rmatot(X13Context& ctx, int otlrev, int nrxy);

// rmotrv.f -- delete every outlier-type regressor whose date falls after
// `begrev` (a RAMP/TLS also counts when its END does) from the regression
// design, appending it to `st` when `lotlrv`. `begxy` is the design's first
// date; `begrev` is the first observation of the revision loop (Beglup).
void rmotrv(X13Context& ctx, const int* begxy, int begrev, int nrxy,
            RevOtlStore& st, bool lotlrv);

// revdrv.f:305's `IF(Notrtl.gt.0) CALL ssprep(Lmodel,F,F)` -- re-take the design
// half of the ssprep snapshot so a structural change survives the next span's
// restor. The CALLER does this, exactly where revdrv does: only after the
// regARIMA rmotrv, and only when that store is non-empty. The x11regression
// branch (revdrv.f:332-350) has no such call -- its changes stick because they
// are saved back into the x11reg store by loadxr(true), which nothing restores.
void rev_snapshot_design(X13Context& ctx);

// chkorv.f -- re-introduce every stored outlier that is now defined, i.e. dated
// at or before `endrev` (this span's MODEL span end, revdrv.f:588's `i-nend`),
// removing it from the store as it goes. When more than one outlier lands
// exactly on the last observation only one can be estimated, so the singularity
// pass keeps the one its type ranks highest (chkorv.f:118-170's `opref`).
// `otlfix` forces the re-added coefficient fixed. Updates the ssprep snapshot
// when `lmdl`, so the next span's restor keeps the re-added regressor.
void chkorv(X13Context& ctx, const int* begxy, int endrev, RevOtlStore& st,
            bool otlfix, int nrxy, bool lmdl);

}  // namespace x13

#endif  // X13_DRIVER_REV_OUTLIER_HPP
