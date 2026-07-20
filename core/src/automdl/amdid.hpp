// amdid.hpp -- amdid.f: automatic ARMA-order identification (Gomez-Maravall
// 1998). Given the differencing orders from iddiff, searches candidate
// (p,q)(P,Q) orders up to maxorder, estimating each (amdid2) and ranking them by
// BIC (bestmd keeps the best five), then applies a balance/parsimony tie-break
// and re-fits the winner. The chosen orders return in irar/irma/isar/isma.
#ifndef X13_AUTOMDL_AMDID_HPP
#define X13_AUTOMDL_AMDID_HPP

#include "common/x13context.hpp"

namespace x13 {

// mdlmch.f: true iff (nrar..nsma) equals one of the best-five models already
// tried (scanning until the first unset DNOTST BIC slot).
bool mdlmch(int nrar, int nrdiff, int nrma, int nsar, int nsdiff, int nsma,
            const int* bstrar, const int* bstrdf, const int* bstrma,
            const int* bstsar, const int* bstsdf, const int* bstsma,
            const double* bstbic);

// bestmd.f: insert the current model (orders + ctx.lkhd.bic2) into the sorted
// best-five lists, displacing higher-BIC entries.
void bestmd(X13Context& ctx, int irar, int irdf, int irma, int isar, int isdf,
            int isma, int* bstrar, int* bstrdf, int* bstrma, int* bstsar,
            int* bstsdf, int* bstsma, double* bstbic);

// amdid2.f: estimate one candidate model -- build it (mdlint/mdlset), optional
// HR initial values (amdest, when hrinit), rgarma, and prlkhd for the BIC. lgo
// returns whether the fit converged with no ARMA error (bestmd is skipped
// otherwise). txy is the differenced+mean-deleted series.
void amdid2(X13Context& ctx, int irar, int irdf, int irma, int isar, int isdf,
            int isma, double* txy, int nelta, bool lmu, bool& lgo);

// amdid.f: the order-search driver. irdf/isdf are the differencing orders (from
// iddiff) on input; irar/irma/isar/isma return the identified ARMA orders. The
// winner is left estimated in ctx (rgarma) with its designation in
// ctx.arima.bstdsn.
void amdid(X13Context& ctx, int& irar, int irdf, int& irma, int& isar, int isdf,
           int& isma, double* trnsrs, int& frstry, int& nefobs, double* a,
           int& na, bool lmu, int lsumm, bool& locok);

}  // namespace x13
#endif  // X13_AUTOMDL_AMDID_HPP
