// amdest.hpp -- Hannan-Rissen initial ARMA estimation for automatic model
// identification (the TRAMO/SEATS HR engine, Gomez-Maravall). amdest is the
// single-candidate estimate wrapper iddiff/amdid call to seed ARMA coefficients
// before the exact-likelihood refit; it is built on the sample ACF (acf/acfar),
// the HR regression (hrest), and the X-13 <-> TRAMO order conversion (cnvmdl).
#ifndef X13_AUTOMDL_AMDEST_HPP
#define X13_AUTOMDL_AMDEST_HPP

#include "common/x13context.hpp"

namespace x13 {

// cnvmdl.f: read the current model's operator structure into the flat TRAMO
// order variables. ipr/ips/iqr/iqs are the nonseasonal/seasonal AR and MA lag
// counts (matched by operator title); idr/ids the differencing orders; the rest
// are derived totals (id, ip, iq combined orders; iprs/iqrs summed AR/MA; n).
void cnvmdl(X13Context& ctx, int& ipr, int& ips, int& idr, int& ids, int& iqr,
            int& iqs, int& id, int& ip, int& iq, int& iprs, int& iqrs, int& n);

// acf.f: sample autocorrelations r(1..nr) of z, Bartlett standard errors se,
// and the Ljung-Box (iqtype==0) or Box-Pierce Q statistics into ctx.autoq
// (c0/qs/qpv/dgf). nr is recomputed when <=0; np is the model parameter count
// for the Q degrees of freedom; lmu subtracts the sample mean. Print deferred.
void acf(X13Context& ctx, const double* z, int nz, int nefobs, double* r,
         double* se, int& nr, int np, int sp, int iqtype, bool lmu, bool lprt);

// acfar.f: autocovariances r(1..m) of the AR-filtered residuals res(1..ndat),
// with m adaptively enlarged from the ip/iq orders and series length; c0 stored
// in ctx.autoq.
void acfar(X13Context& ctx, int& m, double* r, const double* res, int ndat,
           int ip, int iq);

// hrest.f: Hannan-Rissen regression. From the ACF r and (when iq>0) the
// AR-innovation estimates, builds the HR design matrix and OLS-solves for the
// ARMA coefficients hrp. nefobs returns the HR effective-obs count; info>0 flags
// a singular fit (PSNGER), info<0 an HR failure. Third-stage HR is disabled to
// match the vendored source. Print deferred.
void hrest(X13Context& ctx, int iar, const double* x, const double* r,
           double* hrp, int ipr, int ips, int iqr, int iqs, int iq, int iprs,
           int sp, int ndfobs, int& nefobs, bool lprt, int& info);

// amdest.f: estimate one candidate model's ARMA coefficients by HR and scatter
// them (negated) into ctx.mdldat.arimap at offset ardsp. Pure-AR/MA models are
// handled directly; mixed models add an AR-filter + MA-only HR pass. info
// mirrors hrest; on failure ctx.mdldat.armaer is set.
void amdest(X13Context& ctx, double* trnsrs, int nelta, int& nefobs, int ardsp,
            bool lmu, bool lprt, int& info);

}  // namespace x13
#endif  // X13_AUTOMDL_AMDEST_HPP
