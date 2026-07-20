// automd.hpp -- automd.f: the automatic model-selection driver (Gomez-Maravall
// TRAMO method). This is a REDUCED port covering the identification spine that
// is exercised by automdl specs with no auto-transform / aictest / outlier /
// pickmdl preamble: build the default airline model, test the mean (chkmu),
// identify the differencing (iddiff) and ARMA orders (amdid), then re-add the
// mean and re-estimate. Deferred (documented in automdl_scouting.md): the
// regressor AIC-test family (tdaic/lomaic/easaic/usraic/chkchi), automatic
// outlier ID (amidot), pass0, and the model-adequacy retry loop
// (tstmd1/tstmd2/tstodf) -- the last of which can revise the BIC winner (e.g.
// region_north's arimamdl).
#ifndef X13_AUTOMDL_AUTOMD_HPP
#define X13_AUTOMDL_AUTOMD_HPP

#include "common/x13context.hpp"

namespace x13 {

// automd (reduced): drive automatic model identification on the transformed
// series trnsrs (a caller-owned buffer, kept distinct from ctx.series.tsrs which
// rgarma overwrites with residuals). Leaves the identified model estimated in
// ctx (arimap/var/lnlkhd/...) and its designation in ctx.arima.bstdsn. a/na are
// the ARMA residual scratch; frstry/nefobs are regvar/estimation outputs.
void automd(X13Context& ctx, double* trnsrs, int& frstry, int& nefobs,
            double* a, int& na);

}  // namespace x13
#endif  // X13_AUTOMDL_AUTOMD_HPP
