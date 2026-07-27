// amdfct.cpp -- see hpp.
#include "diag/amdfct.hpp"

#include "common/x13context.hpp"
#include "regarima/forecast.hpp"   // fcstxy
#include "transform/transform.hpp" // invfcn
#include "numeric/numeric.hpp"     // dpeq

#include <algorithm>
#include <cmath>
#include <vector>

namespace x13 {

void aape_diagnostics(X13Context& ctx, const double* trnsrs) {
    AapeDiagnostics& out = ctx.aape;
    out = AapeDiagnostics{};

    const model_cmn& m = ctx.model;
    const mdldat_cmn& d = ctx.mdldat;
    const int sp = m.sp;
    const int nspobs = d.nspobs;
    const int nfc = sp;
    int nobsf = 3 * sp;
    const int nobsot = nspobs - nobsf;

    // amdfct.f:55-60 -- not enough observations BEFORE the three-year window to
    // have estimated the model there. The oracle prints a NOTE and returns
    // Fctok=F, which arima.f:904 turns into `aape.mode: none`.
    if (((nobsot - (m.mxdflg + m.mxarlg)) * m.ncxy) + 1 <= 0) return;
    if (nobsot <= 0 || nspobs <= 0) return;

    // amdfct.f:65-67 -- Fctdrp is forced to 0 for the whole diagnostic and
    // restored on the way out; fcstxy reads it.
    const int fdbak = ctx.arima.fctdrp;
    ctx.arima.fctdrp = 0;

    const int fcntyp = ctx.arima.fcntyp;
    const double lam = ctx.arima.lam;
    constexpr int PFCST = 120;   // srslen.prm: 10*PSP

    // amdfct.f:161-184 -- the scale the errors are divided by. `ave` is 1
    // unless the ORIGINAL-scale series dips to or below zero in the window, in
    // which case a percentage is meaningless and the oracle switches to an
    // absolute error scaled by the window's mean magnitude.
    //
    // Ported verbatim including the seed: `ave` is initialised to ONE and then
    // ACCUMULATED into, so the divisor is (1 + sum|x|)/n, not the mean. That
    // one-unit offset is a Census defect but it is only reachable on a series
    // that crosses zero, and it is what the oracle computes.
    int ivalue = 0;
    double ave = 1.0;
    {
        std::vector<double> tmpsrs(static_cast<std::size_t>(nobsf), 0.0);
        for (int i = 0; i < nobsf; ++i) tmpsrs[i] = trnsrs[nobsot + i];
        if (ctx.fxreg.nfxttl > 0)
            for (int i = 0; i < nobsf; ++i) tmpsrs[i] += ctx.fxreg.fixfac(nobsot + i + 1);
        invfcn(ctx, tmpsrs.data(), nobsf, fcntyp, lam, tmpsrs.data());
        if (ctx.error.lfatal) { ctx.arima.fctdrp = fdbak; return; }
        double ad1 = 1.0;
        for (int i = 0; i < nobsf; ++i) ad1 = std::min(ad1, tmpsrs[i]);
        if (ad1 <= 0.0) {
            ivalue = 1;
            double dn = 0.0;
            for (int i = 0; i < nobsf; ++i) { ave += std::fabs(tmpsrs[i]); dn += 1.0; }
            ave /= dn;
            if (dpeq(ave, 0.0)) ave = 1.0;
        }
    }

    // amdfct.f:186-263 -- one pass per year back.
    for (int i = 1; i <= 3; ++i) {
        const int disp = -i * sp;
        const int fctori = nspobs + disp;
        nobsf = std::min(nfc, ctx.extend.nobspf - fctori);
        if (nobsf <= 0) { ctx.arima.fctdrp = fdbak; return; }

        std::vector<double> fcst(PFCST, 0.0), se(PFCST, 0.0), fdiff(PFCST, 0.0);
        fcstxy(ctx, fctori, nfc, fcst.data(), se.data(), fdiff.data());
        if (ctx.error.lfatal) { ctx.arima.fctdrp = fdbak; return; }

        // subset.f -- the ACTUALs are the dependent-variable column of Xy over
        // the forecast rows. Xy is (Ncxy, Nrxy) in the Fortran, i.e. the last
        // element of each row, which is `xy(ncxy*irow)` in this port's flat
        // 1-based buffer.
        std::vector<double> tmpsrs(PFCST, 0.0);
        for (int k = 0; k < nobsf; ++k) {
            const int irow = fctori + 1 + k;
            tmpsrs[k] = d.xy(m.ncxy * irow);
        }

        // amdfct.f:246-251 -- a FIXED regressor's contribution was subtracted
        // out of both the series and the forecast during estimation; put it
        // back into both before the error is formed.
        if (ctx.fxreg.nfxttl > 0) {
            for (int k = 0; k < nobsf; ++k) {
                const double f = ctx.fxreg.fixfac(fctori + 1 + k);
                tmpsrs[k] += f;
                fcst[k] += f;
            }
        }

        invfcn(ctx, fcst.data(), nobsf, fcntyp, lam, fcst.data());
        if (ctx.error.lfatal) { ctx.arima.fctdrp = fdbak; return; }
        invfcn(ctx, tmpsrs.data(), nobsf, fcntyp, lam, tmpsrs.data());
        if (ctx.error.lfatal) { ctx.arima.fctdrp = fdbak; return; }

        double dn = 0.0, acc = 0.0;
        for (int k = 0; k < nobsf; ++k) {
            double e = tmpsrs[k] - fcst[k];
            if (ivalue == 0) e /= tmpsrs[k];
            acc += std::fabs(e);
            dn += 1.0;
        }
        out.mape[i - 1] = (acc * 100.0) / (dn * ave);
    }

    out.mape[3] = (out.mape[0] + out.mape[1] + out.mape[2]) / 3.0;
    ctx.arima.fctdrp = fdbak;
    out.ok = true;
}

}  // namespace x13
