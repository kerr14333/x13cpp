// estimate.cpp -- olsreg.f / resid.f (regression solve + residuals). Faithful
// ports of the vendored oracle Fortran.
#include "regarima/estimate.hpp"

#include <string>

#include "numeric/numeric.hpp"      // xprmx, dppfa, dcopy, daxpy
#include "specparse/specparse.hpp"  // copy, abend, errhdr, writln, stdio::STDERR
#include "gen/model.hpp"            // prm::DIFF, prm::MA

namespace x13 {

// olsreg.f -- normal equations then Cholesky back-substitution. The packed
// factor chlxpx after dppfa holds [chol(X'X); z; sqrt(RSS)]; the betas solve
// L'b = z walking the packed columns from the bottom (xelt) upward.
void olsreg(X13Context& ctx, const double* xy, int nrxy, int ncxy, int pcxy,
            double* b, double* chlxpx, int pxpx, int& info) {
    if (ncxy * (ncxy + 1) / 2 > pxpx) {
        errhdr(ctx);
        writln(ctx,
               " Elements needed for [X:y]'[X:y] exceed PXPX (" +
                   std::to_string(ncxy) + "*(" + std::to_string(ncxy) +
                   "+1)/2 > " + std::to_string(pxpx) + ")",
               stdio::STDERR, ctx.units.mt2, true);
        abend(ctx);
        return;
    }
    // Form X'X and X'y as the packed [X:y]'[X:y].
    xprmx(xy, nrxy, ncxy, pcxy, chlxpx);
    dppfa(chlxpx, ncxy, info);
    if (info <= 0 || info == ncxy) {
        int nb = ncxy - 1;
        int xelt = nb * ncxy / 2;
        copy(chlxpx + xelt, nb, 1, b);  // Chlxpx(xelt+1) -> b
        for (int i = nb; i >= 1; --i) {
            b[i - 1] = b[i - 1] / chlxpx[xelt - 1];
            xelt = xelt - i;
            daxpy(i - 1, -b[i - 1], chlxpx + xelt, 1, b, 1);
        }
        info = 0;  // reset when y is linearly dependent on X
    }
}

// resid.f -- rsd = y (+/-) X*b. addsub = sign(1,fac); the y column is xy(pc)
// with stride pc, each regression column icol added with stride pc.
void resid(X13Context& ctx, const double* xy, int nr, int nc, int pc, int begcol,
           int endcol, double fac, const double* b, double* rsd) {
    if (nc == 0 || endcol + 1 == begcol) {
        dcopy(nr, xy + (pc - 1), pc, rsd, 1);
    } else if (begcol < 1 || endcol > nc || endcol < begcol) {
        errhdr(ctx);
        writln(ctx, " Column error, 1<=begcol<=endcol<=nc", stdio::STDERR,
               ctx.units.mt2, true);
        abend(ctx);
        return;
    } else {
        double addsub = (fac < 0.0) ? -1.0 : 1.0;  // sign(ONE,Fac)
        dcopy(nr, xy + (pc - 1), pc, rsd, 1);
        for (int icol = begcol; icol <= endcol; ++icol)
            daxpy(nr, addsub * b[icol - 1], xy + (icol - 1), pc, rsd, 1);
    }
}

// upespm.f -- scatter estprm into arimap, skipping fixed lags. estptr advances
// only for non-fixed lags, so estprm is a dense vector of just the free params.
void upespm(X13Context& ctx, const double* estprm) {
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    int estptr = 0;
    for (int iflt = prm::DIFF; iflt <= prm::MA; ++iflt) {
        int begopr = m.mdl(iflt - 1);
        int endopr = m.mdl(iflt) - 1;
        for (int iopr = begopr; iopr <= endopr; ++iopr) {
            int beglag = m.opr(iopr - 1);
            int endlag = m.opr(iopr) - 1;
            for (int ilag = beglag; ilag <= endlag; ++ilag) {
                if (!m.arimaf(ilag)) {
                    estptr = estptr + 1;
                    d.arimap(ilag) = estprm[estptr - 1];
                }
            }
        }
    }
}

}  // namespace x13
