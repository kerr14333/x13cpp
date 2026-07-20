// amdest.cpp -- cnvmdl.f / acf.f / acfar.f / hrest.f / amdest.f: the Hannan-
// Rissen initial-estimate engine for automatic model identification. See
// amdest.hpp for per-routine roles. Indexing follows the Fortran verbatim
// (1-based arithmetic preserved via -1 offsets); olsreg/arflt/copy are reused.
#include "automdl/amdest.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "numeric/numeric.hpp"       // chisq
#include "regarima/estimate.hpp"     // olsreg
#include "regarima/armafilt.hpp"     // arflt
#include "specparse/specparse.hpp"   // setdp (inline), copy, getstr
#include "gen/model.hpp"             // prm::AR, MA, PARIMA, PXPX, PSNGER
#include "gen/srslen.hpp"            // prm::PLEN
#include "gen/notset.hpp"            // prm::DNOTST

namespace x13 {

namespace {
constexpr int PR = prm::PLEN / 4;   // 255, autoq vector length
// n35 = INT( (ln n)^2 ), the TRAMO ACF-length heuristic.
int n35len(int n) {
    double l = std::log(static_cast<double>(n));
    return static_cast<int>(l * l);
}
}  // namespace

void cnvmdl(X13Context& ctx, int& ipr, int& ips, int& idr, int& ids, int& iqr,
            int& iqs, int& id, int& ip, int& iq, int& iprs, int& iqrs, int& n) {
    using namespace prm;
    auto& m = ctx.model;
    idr = m.nnsedf;
    ids = m.nseadf;
    id = idr + m.sp * ids;
    ipr = 0;
    ips = 0;
    iqr = 0;
    iqs = 0;
    std::string ttl;
    int ntmp;
    for (int iflt = AR; iflt <= MA; ++iflt) {
        int begopr = m.mdl(iflt - 1);
        int endopr = m.mdl(iflt) - 1;
        for (int iopr = begopr; iopr <= endopr; ++iopr) {
            int nlag = m.opr(iopr) - m.opr(iopr - 1);
            getstr(ctx, m.oprttl.data(), m.oprptr.data(), m.noprtl, iopr, ttl,
                   ntmp);
            if (ctx.error.lfatal) return;
            if (ttl == "Nonseasonal AR")
                ipr = nlag;
            else if (ttl == "Seasonal AR")
                ips = nlag;
            else if (ttl == "Nonseasonal MA")
                iqr = nlag;
            else if (ttl == "Seasonal MA")
                iqs = nlag;
        }
    }
    ip = ipr + m.sp * ips;
    iq = iqr + m.sp * iqs;
    iprs = ipr + ips;
    iqrs = iqr + iqs;
    n = iprs + iqrs;
}

void acf(X13Context& ctx, const double* z, int nz, int nefobs, double* r,
         double* se, int& nr, int np, int sp, int iqtype, bool lmu, bool lprt) {
    (void)lprt;  // ACF table print deferred with the rest of the print engine
    auto& aq = ctx.autoq;
    if (nr <= 0) nr = std::min(3 * sp, static_cast<int>(nz / 4.0 + 0.99));

    double mu = 0.0;
    aq.c0 = 0.0;
    if (lmu)
        for (int k = 0; k < nz; ++k) mu += z[k];
    mu = mu / nz;
    for (int k = 0; k < nz; ++k) aq.c0 += (z[k] - mu) * (z[k] - mu);

    if (aq.c0 <= 0.0) {
        // Zero-variance series: no ACF is defined. The NOTE print is deferred;
        // flag every p-value as not-set and leave r/se untouched (as Fortran).
        setdp(prm::DNOTST, PR, aq.qpv.data());
        return;
    }

    aq.c0 = aq.c0 / nz;
    std::vector<double> c(nr, 0.0);
    double sq = 0.0;
    for (int i = 1; i <= nr; ++i) {
        c[i - 1] = 0.0;
        for (int j = i + 1; j <= nz; ++j)
            c[i - 1] += (z[j - 1] - mu) * (z[j - i - 1] - mu);
        c[i - 1] = c[i - 1] / nz;
        r[i - 1] = c[i - 1] / aq.c0;
        if (iqtype == 0) {
            sq += r[i - 1] * r[i - 1] / (nefobs - i);
            aq.qs(i) = sq * nefobs * (nefobs + 2);
        } else {
            sq += r[i - 1] * r[i - 1];
            aq.qs(i) = sq * nefobs;
        }
        aq.dgf(i) = std::max(0, i - np);
        if (aq.dgf(i) > 0)
            aq.qpv(i) = chisq(aq.qs(i), aq.dgf(i));
        else
            aq.qpv(i) = 0.0;
    }
    // Bartlett standard errors.
    se[0] = 1.0 / std::sqrt(static_cast<double>(nz));
    double sr = 0.0;
    for (int i = 1; i <= nr - 1; ++i) {
        sr += r[i - 1] * r[i - 1];
        se[i] = std::sqrt((1.0 + 2.0 * sr) / static_cast<double>(nz));
    }
}

void acfar(X13Context& ctx, int& m, double* r, const double* res, int ndat,
           int ip, int iq) {
    auto& aq = ctx.autoq;
    int n = ndat;
    int m1 = std::max(m, ip + iq + 1);
    int n35 = n35len(n);
    m = std::max(m1, n35);
    if (m >= n) m = std::min(m1, n - n / 4);

    aq.c0 = 0.0;
    for (int i = 1; i <= n; ++i) aq.c0 += res[i - 1] * res[i - 1];
    aq.c0 = aq.c0 / n;
    for (int k = 1; k <= m; ++k) {
        r[k - 1] = 0.0;
        for (int i = k + 1; i <= n; ++i) r[k - 1] += res[i - 1] * res[i - 1 - k];
        r[k - 1] = r[k - 1] / (static_cast<double>(n) * aq.c0);
    }
}

void hrest(X13Context& ctx, int iar, const double* x, const double* r,
           double* hrp, int ipr, int ips, int iqr, int iqs, int iq, int iprs,
           int sp, int ndfobs, int& nefobs, bool lprt, int& info) {
    using namespace prm;
    (void)lprt;  // HR failure message print deferred
    const double c0 = ctx.autoq.c0;

    std::vector<double> tmpchl(PXPX, 0.0);
    std::vector<double> phat(PR, 0.0), pcf(PR, 0.0), tmp(PLEN, 0.0),
        ahat(PLEN, 0.0), ptmp(PARIMA, 0.0);

    // ---- innovation estimates ahat when MA terms are present ----
    if (iq > 0) {
        pcf[0] = r[0];
        double pv = c0 * (1.0 - pcf[0] * pcf[0]);
        phat[0] = pcf[0];
        for (int i = 2; i <= iar; ++i) {
            double psum = 0.0;
            for (int j = 1; j <= i - 1; ++j) {
                psum += phat[j - 1] * (r[i - j - 1] * c0);
                tmp[j - 1] = phat[i - j - 1];
            }
            pcf[i - 1] = ((r[i - 1] * c0) - psum) / pv;
            pv = pv * (1.0 - pcf[i - 1] * pcf[i - 1]);
            phat[i - 1] = pcf[i - 1];
            for (int j = 1; j <= i - 1; ++j)
                phat[j - 1] -= pcf[i - 1] * tmp[j - 1];
        }
        for (int i = 1; i <= iar; ++i) {
            ahat[i - 1] = x[i - 1];
            for (int j = 1; j <= iar; ++j)
                if ((i - j) > 0) ahat[i - 1] -= phat[j - 1] * x[i - j - 1];
        }
        for (int i = iar + 1; i <= ndfobs; ++i) {
            ahat[i - 1] = x[i - 1];
            for (int j = 1; j <= iar; ++j) ahat[i - 1] -= phat[j - 1] * x[i - j - 1];
        }
    }

    int ncol = ipr + ips * (ipr + 1) + iqr + iqs * (iqr + 1);
    int nc1 = ncol + 1;
    int mxlg = std::max(ipr + sp * ips, iqr + sp * iqs);
    nefobs = ndfobs - mxlg;

    std::vector<double> xmat(static_cast<std::size_t>(nefobs) * nc1, 0.0);
    for (int i = 1 + mxlg; i <= ndfobs; ++i) {
        int i2 = (i - mxlg - 1) * nc1;
        for (int j = 1; j <= ipr; ++j) xmat[j - 1 + i2] = -x[i - j - 1];
        for (int j = 1; j <= ips; ++j) {
            int jj = (ipr + 1) * j;
            xmat[jj - 1 + i2] = -x[i - j * sp - 1];
            for (int k = 1; k <= ipr; ++k)
                xmat[jj + k - 1 + i2] = -x[i - j * sp - k - 1];
        }
        int kk = ipr + (ipr + 1) * ips;
        for (int j = 1; j <= iqr; ++j) xmat[kk + j - 1 + i2] = ahat[i - j - 1];
        for (int j = 1; j <= iqs; ++j) {
            int jj = kk + (iqr + 1) * j;
            xmat[jj - 1 + i2] = ahat[i - j * sp - 1];
            for (int k = 1; k <= iqr; ++k)
                xmat[jj + k - 1 + i2] = ahat[i - j * sp - k - 1];
        }
        xmat[nc1 - 1 + i2] = x[i - 1];
    }

    olsreg(ctx, xmat.data(), nefobs, nc1, nc1, ptmp.data(), tmpchl.data(), PXPX,
           info);
    if (ctx.error.lfatal) return;
    if (info > 0) {
        info = PSNGER;
        return;
    }

    for (int i = 1; i <= ipr; ++i) hrp[i - 1] = ptmp[i - 1];
    for (int i = 1; i <= ips; ++i) hrp[ipr + i - 1] = ptmp[(ipr + 1) * i - 1];
    for (int i = 1; i <= iqr; ++i)
        hrp[iprs + i - 1] = ptmp[ipr + ips * (ipr + 1) + i - 1];
    for (int i = 1; i <= iqs; ++i)
        hrp[iprs + iqr + i - 1] = ptmp[ipr + ips * (ipr + 1) + (iqr + 1) * i - 1];
    // Third-stage HR omitted (disabled in the vendored source).
}

void amdest(X13Context& ctx, double* trnsrs, int nelta, int& nefobs, int ardsp,
            bool lmu, bool lprt, int& info) {
    using namespace prm;
    constexpr int KP = 50;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    int ipr, ips, idr, ids, iqr, iqs, id, ip, iq, iprs, iqrs, n;
    cnvmdl(ctx, ipr, ips, idr, ids, iqr, iqs, id, ip, iq, iprs, iqrs, n);
    if (ctx.error.lfatal) return;
    if ((ip + iq) == 0) return;

    double hrp[PARIMA];
    for (int i = 1; i <= iprs + iqrs; ++i) hrp[i - 1] = 0.0;

    int n35 = n35len(nelta);
    int iar = n35;
    int m1 = std::max(iar, std::max(ip, 2 * iq));
    iar = std::max(m1, n35);
    int n1 = nelta;
    if (iar >= n1) iar = std::min(m1, n1 - n1 / 4);
    if (iar > KP) iar = KP;

    std::vector<double> r(PR, 0.0), tmp(PR, 0.0);
    acf(ctx, trnsrs, nelta, nefobs, r.data(), tmp.data(), iar, n, m.sp, 0, lmu,
        false);

    info = 0;
    hrest(ctx, iar, trnsrs, r.data(), hrp, ipr, ips, iqr, iqs, iq, iprs, m.sp,
          nelta, nefobs, lprt, info);

    if (info == 0) {
        for (int i = 1; i <= ipr; ++i) d.arimap(ardsp + i) = -hrp[i - 1];
        for (int i = 1; i <= ips; ++i) d.arimap(ardsp + ipr + i) = -hrp[ipr + i - 1];
        for (int i = 1; i <= iqr; ++i)
            d.arimap(ardsp + iprs + i) = -hrp[iprs + i - 1];
        for (int i = 1; i <= iqs; ++i)
            d.arimap(ardsp + iprs + iqr + i) = -hrp[iprs + iqr + i - 1];
    } else {
        if (info < 0) d.armaer = -info;
        return;
    }

    // Mixed AR+MA model: AR-filter the series, then re-estimate the MA part.
    if (ip > 0 && iq > 0) {
        std::vector<double> tmpsrs(PLEN, 0.0);
        copy(trnsrs, nelta, -1, tmpsrs.data());
        arflt(nelta, d.arimap.data(), m.arimal.data(), m.opr.data(),
              m.mdl(AR - 1), m.mdl(AR) - 1, tmpsrs.data(), n1);
        n35 = n35len(n1);
        iar = n35;
        m1 = std::max(iar, 2 * iq);
        iar = std::max(m1, n35);
        if (iar >= n1) iar = std::min(m1, n1 - n1 / 4);
        acfar(ctx, iar, r.data(), tmpsrs.data(), nelta - ip, ip, iq);
        hrest(ctx, iar, tmpsrs.data(), r.data(), hrp, 0, 0, iqr, iqs, iq, iprs,
              m.sp, nelta - ip, nefobs, lprt, info);
        if (info == 0) {
            for (int i = 1; i <= iqr; ++i)
                d.arimap(ardsp + iprs + i) = -hrp[iprs + i - 1];
            for (int i = 1; i <= iqs; ++i)
                d.arimap(ardsp + iprs + iqr + i) = -hrp[iprs + iqr + i - 1];
        } else {
            if (info < 0) d.armaer = -info;
            return;
        }
    }
}

}  // namespace x13
