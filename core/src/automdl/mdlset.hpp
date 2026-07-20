// mdlset.hpp -- programmatic ARIMA model construction for automatic model
// identification (automdl). Where getmdl.f builds the model operators from the
// spec's token stream, mdlset.f builds them from six explicit order counts
// (nonseasonal/seasonal AR, differencing, MA). iddiff / amdid call this to lay
// down each trial model before estimating it. mdlint resets the operator state
// between trials; setopr is the no-lexer sibling of getopr; mkmdsn builds the
// (p d q)(P D Q) designation string.
#ifndef X13_AUTOMDL_MDLSET_HPP
#define X13_AUTOMDL_MDLSET_HPP

#include <string>
#include "common/x13context.hpp"

namespace x13 {

// mdlint.f: clear the ARIMA operator/coefficient state to an empty model
// (Mdl(AR)=Mdl(MA)=1, all lag/coef/fix vectors zeroed). Called at the top of
// each iddiff round before mdlset lays down the trial orders.
void mdlint(X13Context& ctx);

// mkmdsn.f: write the "(p d q)(P D Q)" model designation into model.mdldsn,
// setting model.nmddcr to its character length. The seasonal group is omitted
// when all three seasonal orders are 0.
void mkmdsn(X13Context& ctx, int nrar, int nrdiff, int nrma, int nsar,
            int nsdiff, int nsma);

// setopr.f: fill one operator's coefficient/lag/fix vectors for a plain order
// (no token input, unlike getopr). For DIFF, builds (1-B)^nd via polyml and
// returns the expanded degree in ncoef; for AR/MA, lags 1..ncoef with DNOTST
// coefficients and unfixed. naimcf accumulates the running coefficient count;
// locok/inptok flag order-overflow errors.
void setopr(X13Context& ctx, int optype, double* coef, int* lag, bool* fix,
            int& ncoef, int nd, int& naimcf, bool& locok, bool& inptok);

// mdlset.f: build the full trial model from the six order counts, inserting each
// nonzero operator (nonseasonal then seasonal, AR/DIFF/MA) into the model state
// via setopr + insopr. inptok is cleared on any construction error.
void mdlset(X13Context& ctx, int nrar, int nrdiff, int nrma, int nsar,
            int nsdiff, int nsma, bool& inptok);

}  // namespace x13
#endif  // X13_AUTOMDL_MDLSET_HPP
