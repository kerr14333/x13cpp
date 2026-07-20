// adqtst.hpp -- model-adequacy tests for automatic model selection (automd.f's
// finalization). This module will grow to hold tstmd1 (Ljung-Box adequacy),
// tstmd2 (unit-root nearness), and testodf (over-differencing); it starts with
// mdlchk, the shared Ljung-Box + residual-mean statistic used by all three and
// by automd's redo path.
#ifndef X13_AUTOMDL_ADQTST_HPP
#define X13_AUTOMDL_ADQTST_HPP

#include "common/x13context.hpp"

namespace x13 {

// mdlchk.f: Ljung-Box diagnostics for the current model's residuals a(1..na).
// Chooses the LB lag count bldf from the seasonal period (24 monthly, 8 for
// Sp==1, 4*Sp otherwise; a small-sample quarterly special case), capping it at
// nefobs/2 when there are too few observations, then runs acf over the residual
// tail to fill ctx.autoq. Returns blpct = 1 - Qpv(bldf), blq = Qs(bldf), the
// residual-mean t-value rtval, and rvr = sqrt(Var).
void mdlchk(X13Context& ctx, const double* a, int na, int nefobs, double& blpct,
            double& blq, int& bldf, double& rvr, double& rtval);

// tstmd2.f: unit-root-nearness / insignificant-parameter reduction. Checks the
// ARMA t-statistics (armats) against Tsig and the coefficient magnitude against
// a size-dependent cmin; where a parameter is insignificant AND its operator has
// no near-unit root (chkurt), it drops the highest lag of that operator and
// rebuilds the model (mdlint/mdlset). Never reduces a total-order-1 model or one
// with any unit root. nnsig returns the number of parameters removed; ipr/iqr/
// ips/iqs are updated in place. nz is the series length (sets cmin).
void tstmd2(X13Context& ctx, int& nnsig, int nz, int& ipr, int& iqr, int& ips,
            int& iqs);

// testodf.f: nonseasonal (and, when Lsovdf, seasonal) OVER-differencing test.
// When the model carries regular differencing AND regular MA and the sum of the
// regular MA coefficients is within MALIM (0.001) of 1 -- an MA unit root that
// cancels a difference -- it drops one regular difference and one regular MA lag,
// adds a Constant (when Lchkmu), rebuilds and re-estimates the model, then
// rechecks the mean (chkmu). redomd reports whether the model was changed.
// Deferred: the Lsovdf seasonal-regressor path (needs sftest) and the automatic-
// outlier branches (amidot/clrotl), both inactive for the no-outlier default.
void testodf(X13Context& ctx, double* trnsrs, int& frstry, int& nefobs,
             double* a, int& na, int& lpr, int& ldr, int& lqr, int& lps,
             int& lds, int& lqs, int kstep, bool& redomd, bool& argok);

// bkdfmd.f (reduced): back up (backup=true) or restore (false) the fitted-model
// state into ctx.ss2rv -- the ARIMA coefficients + covariance (arimap/b/var/
// chlxpx/chlgpg/armacm/lndtcv/arimaf), the regression group structure
// (grp/grpptr/colptr/titles/rgvrtp/regfx, nb/ncxy/ncoltl/ngrp/ngrptl/nrxy/
// iregfx), the differencing extents (nintvl/nextvl/mxdflg/mxarlg/mxmalg,
// lar/lma), and priadj. automd uses this to save the default model before
// identification and restore it when tstmd1 reverts. Deferred (constant during
// no-holiday/no-outlier model-ID, so their backup is a no-op): the holiday/TD/
// outlier-adjustment fields (Adjtd/Adjhol/Fin*/Ltst*/Picktd/Ncusrx).
void bkdfmd(X13Context& ctx, bool backup);

// tstmd1.f (reduced): model-adequacy test comparing the identified model to the
// default airline model. Reduces insignificant ARMA lags, then -- on any of five
// adequacy conditions (near-unit AR, Ljung-Box comparison vs the default's
// pdfm/rsddfm) -- reverts to the airline default (0 1 1)(0 1 1) via bkdfmd.
// pdfm/rsddfm/rtval and tair are the DEFAULT model's mdlchk stats + MA t-stats,
// captured before identification. Updates lpr..lds in place. Deferred: the
// picktd/prior-restore branch (inactive without holiday/TD auto-selection).
void tstmd1(X13Context& ctx, double* trnsrs, int& frstry, double* a, int& na,
            int& nefobs, double pdfm, double rsddfm, double rtval, int& lpr,
            int& lps, int& lqr, int& lqs, int& ldr, int& lds, bool& lmu,
            const double* adj0, const double* trns0, const double* tair);

}  // namespace x13
#endif  // X13_AUTOMDL_ADQTST_HPP
