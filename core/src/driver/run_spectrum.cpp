// run_spectrum.cpp -- spectrum{} periodogram tables (spcdrv.f, increment 1).
//
// Faithful port of the spcdrv.f periodogram path for the three data-based
// save tables sp0/sp1/sp2. The frequency grid is mkfreq.f; the detrending is
// gendff.f (log + regular differencing); the periodogram is spgrh2.f. Peaks,
// warnings, plots, the residual/Tukey/arspec paths are follow-on increments.
#include "driver/run_spectrum.hpp"

#include <cmath>
#include <vector>

#include "common/x13context.hpp"
#include "specparse/specparse.hpp"   // addate, dfdate
#include "x11/x11filt.hpp"           // addmul
#include "gen/notset.hpp"            // prm::NOTSET

namespace x13 {
namespace {

constexpr int PLEN = 1020;

// mkfreq.f: the 61-point frequency grid over which the spectrum is computed
// (Frq(1..61)). Peakwd defaults to 1, Lfqalt/Lprsfq default false. The trading-
// day peak substitutions (sp==12) shift six near-.3482/.432 entries. Returns a
// 0-based vector where frq[i] == Fortran Frq(i+1).
std::vector<double> mkfreq(int sp, int peakwd) {
    std::vector<double> frq(61);
    for (int i = 0; i < 61; ++i) frq[i] = static_cast<double>(i) / 120.0;
    const double f2 = frq[1];  // Frq(2) == 1/120
    if (!(sp == 12)) return frq;  // quarterly TD grid handled in a later increment
    // Frq(43-Peakwd) .. Frq(43+Peakwd) around .3482, Frq(53..) around .432.
    // 1-based Fortran indices -> subtract 1 for the 0-based vector.
    if (peakwd != 2) frq[(43 - peakwd) - 1] = 0.3482 - f2 * peakwd;
    frq[43 - 1] = 0.3482;
    frq[(43 + peakwd) - 1] = 0.3482 + f2 * peakwd;
    if (peakwd != 2) frq[(53 - peakwd) - 1] = 0.432 - f2 * peakwd;
    frq[53 - 1] = 0.432;
    frq[(53 + peakwd) - 1] = 0.432 + f2 * peakwd;
    return frq;
}

// gendff.f: optionally log (natural), then difference `thisd` times, high->low.
// src/dst are 1-based (dst[i-1]); fills dst over [pos1ob,posfob]; valid
// differenced data begins at pos1ob+thisd.
void gendff(const double* src, int pos1ob, int posfob, double* dst,
            bool taklog, int thisd) {
    for (int i = pos1ob; i <= posfob; ++i)
        dst[i - 1] = taklog ? std::log(src[i - 1]) : src[i - 1];
    int df1ob = pos1ob;
    for (int d = 0; d < thisd; ++d) {
        ++df1ob;
        for (int j = posfob; j >= df1ob; --j)
            dst[j - 1] = dst[j - 1] - dst[j - 2];
    }
}

// spgrh2.f: periodogram of x over [n1,n2] (1-based) at the given frequencies;
// Sxx(i) = (Sum x*cos)^2 + (Sum x*sin)^2 over sqrt(n), then 10*log10 if ldecbl.
void spgrh2(const double* x, const std::vector<double>& frq, int n1, int n2,
            bool ldecbl, std::vector<double>& sxx) {
    const int n = n2 - n1 + 1;
    const double sq = std::sqrt(static_cast<double>(n));
    const double two_pi = 2.0 * 3.14159265358979;
    sxx.assign(frq.size(), 0.0);
    for (std::size_t i = 0; i < frq.size(); ++i) {
        double sumc = 0.0, sums = 0.0;
        for (int j = n1; j <= n2; ++j) {
            const int k = j - n1;
            const double ang = two_pi * frq[i] * k;
            sumc += x[j - 1] * std::cos(ang);
            sums += x[j - 1] * std::sin(ang);
        }
        sumc /= sq;
        sums /= sq;
        double s = sumc * sumc + sums * sums;
        if (ldecbl) s = 10.0 * std::log10(s);
        sxx[i] = s;
    }
}

// smeadl.f + sautco.f: mean-delete x[n1..n2] in place, then the biased
// autocovariance cxx[0..lagh1-1] (crosco). Thtapr defaults to 0 (no Tukey-Hanning
// taper) on this path. Returns false if cxx[0]==0 (degenerate). x is modified.
bool sautco(double* x, int n1, int n2, int n, int lagh1,
            std::vector<double>& cxx) {
    double sumf = 0.0;
    for (int i = n1; i <= n2; ++i) sumf += x[i - 1];
    const double xmean = sumf / n;
    for (int i = n1; i <= n2; ++i) x[i - 1] -= xmean;
    cxx.assign(static_cast<std::size_t>(lagh1), 0.0);
    for (int i = 0; i < lagh1; ++i) {
        double t = 0.0;
        for (int j = n1; j <= n2 - i; ++j) t += x[(j + i) - 1] * x[j - 1];
        cxx[i] = t / n;
    }
    return cxx[0] != 0.0;
}

// sicp2.f: Levinson-Durbin AR fit up to order l = l1-1. NOTE the Census 5/15/80
// modification: although the loop tracks an AIC-selected order (Moar/Osd), the
// label-10 tail unconditionally OVERWRITES it with the FULL order l -- coef =
// -a[1..l], osd = the order-l innovation variance. The AIC selection is dead
// code; the returned model is always order l. (Ported bug: reproduced verbatim.)
void sicp2(const std::vector<double>& cyy, int l1, int n,
           std::vector<double>& coef, int& moar, double& osd) {
    const double cst1 = 1.0, cst2 = 2.0, cst01 = 0.00001;
    const int l = l1 - 1;
    double sd = cyy[0];
    const double an = n;
    double oaic = an * std::log(sd);
    osd = sd;
    moar = 0;
    std::vector<double> a(static_cast<std::size_t>(l + 2), 0.0);
    std::vector<double> b(static_cast<std::size_t>(l + 2), 0.0);
    double se = cyy[1];
    for (int m = 1; m <= l; ++m) {
        const double sdr = sd / cyy[0];
        if (sdr < cst01) break;   // GO TO 10
        const int mp1 = m + 1;
        const double d = se / sd;
        a[m] = d;
        sd = (cst1 - d * d) * sd;
        const double aic = an * std::log(sd) + cst2 * static_cast<double>(m);
        if (m != 1)
            for (int i = 1; i <= m - 1; ++i) a[i] = a[i] - d * b[i];
        for (int i = 1; i <= m; ++i) b[i] = a[mp1 - i];
        if (oaic >= aic) { oaic = aic; osd = sd; moar = m; }
        if (m != l) {
            se = cyy[m + 1];
            for (int i = 1; i <= m; ++i) se -= b[i] * cyy[i];
        }
    }
    // label 10: the full-order-l override (discards the AIC selection above).
    osd = sd;
    moar = l;
    coef.assign(static_cast<std::size_t>(l + 1), 0.0);
    for (int i = 1; i <= l; ++i) coef[i] = -a[i];
}

// spgrh.f: the AR-spectrum estimator (the arspec type). Mean-delete + auto-
// covariance (sautco), full-order AR fit (sicp2), then the AR transfer-function
// spectrum sgme2 / |1 + sum coef[k] exp(-i 2 pi k f)|^2 at the given frequencies.
// Returns false (leave sxx untouched) when sautco is degenerate.
bool spgrh(const double* yy, const std::vector<double>& frq, int n1, int n2,
           int nspfrq, int sp, int mxarsp, bool ldecbl, std::vector<double>& sxx) {
    const double PI = 3.14159265358979;
    const int n = n2 - n1 + 1;
    std::vector<double> x(PLEN, 0.0);
    for (int i = n1; i <= n2; ++i) x[i - 1] = yy[i - 1];
    const int h = nspfrq - 1;
    const int lagh1 = std::min(n - 1, h) + 1;
    std::vector<double> cxx;
    if (!sautco(x.data(), n1, n2, n, lagh1, cxx)) return false;
    int ifpl = (mxarsp == prm::NOTSET) ? 30 * sp / 12 : mxarsp;
    ifpl = std::min(ifpl, n - 1);
    std::vector<double> coef;
    int l = 0;
    double sgme2 = 0.0;
    sicp2(cxx, ifpl + 1, n, coef, l, sgme2);
    sxx.assign(static_cast<std::size_t>(nspfrq), 0.0);
    for (int i = 0; i < nspfrq; ++i) {
        double c2 = 1.0, s2 = 0.0;
        for (int k = 1; k <= l; ++k) c2 += coef[k] * std::cos(2.0 * k * PI * frq[i]);
        for (int k = 1; k <= l; ++k) s2 += coef[k] * std::sin(2.0 * k * PI * frq[i]);
        double pxx = sgme2 / (c2 * c2 + s2 * s2);
        if (ldecbl) { if (pxx < 0.0) pxx = -pxx; pxx = 10.0 * std::log10(pxx); }
        sxx[i] = pxx;
    }
    return true;
}

// getTPeaks window size (specpeak.f:560-577), sp==12 branch: the Tukey window m
// as a function of series length nz. Returns -1 when no Tukey table is produced.
int tukey_window(int nz, bool ltk120) {
    if (ltk120 && nz >= 120) return 120;
    if (nz >= 120) return 112;
    if (nz >= 80) return 79;
    return -1;
}

// The Tukey-smoothed spectrum of the 1-based series x[n1..n2] (getTPeaks ->
// getWind(Tukey) + covWind, ansub11.f). Fills st[0..m/2] with 10*log10(|p|) and
// frq[0..m/2] with i/m (savstp.f's single-precision float division), where m is
// the getTPeaks window. Returns false (no table) when nz<=80 or m<0. crosco.f:
// biased autocovariance c(i)=(1/nz)*sum x(j+i)x(j), no mean removal.
bool tukey_spectrum(const double* x, int n1, int n2, bool ltk120,
                    std::vector<double>& st, std::vector<double>& frq) {
    const int nz = n2 - n1 + 1;
    if (nz <= 80) return false;   // spcdrv.f gate: nsrs > 80
    const int m = tukey_window(nz, ltk120);
    if (m < 0) return false;
    // Repack to 0-based x[0..nz-1] = series[n1..n2].
    std::vector<double> xx(static_cast<std::size_t>(nz));
    for (int j = 0; j < nz; ++j) xx[j] = x[(n1 - 1) + j];
    // crosco: c[0..m].
    std::vector<double> c(static_cast<std::size_t>(m + 1), 0.0);
    for (int i = 0; i <= m; ++i) {
        double t = 0.0;
        for (int j = 0; j < nz - i; ++j) t += xx[j + i] * xx[j];
        c[i] = t / nz;
    }
    // Tukey window w[0..m] (getWind ind=2).
    std::vector<double> w(static_cast<std::size_t>(m + 1));
    for (int j = 0; j <= m; ++j)
        w[j] = 0.5 * (1.0 + std::cos(3.14159265358979 * j / m));
    // covWind (pm=nw2=600 -> mm=m): p[0]=sum c[j]w[j] (factor 1 -- the Census
    // quirk), p[k]=c0*w0 + sum_{j=1}^m 2 c[j] w[j] cos(2pi j k / m).
    const double pi2 = 6.28318530717959;
    const int half = m / 2;
    std::vector<double> p(static_cast<std::size_t>(half + 1), 0.0);
    for (int j = 0; j <= m; ++j) p[0] += c[j] * w[j];
    for (int k = 1; k <= half; ++k) {
        double s = c[0] * w[0];
        for (int j = 1; j <= m; ++j) s += 2.0 * c[j] * w[j] * std::cos(pi2 * j * k / m);
        p[k] = s;
    }
    st.assign(static_cast<std::size_t>(half + 1), 0.0);
    frq.assign(static_cast<std::size_t>(half + 1), 0.0);
    for (int i = 0; i <= half; ++i) {
        st[i] = 10.0 * std::log10(std::fabs(p[i]));
        frq[i] = static_cast<double>(static_cast<float>(i) / static_cast<float>(m));
    }
    return true;
}

}  // namespace

bool run_spectrum(X13Context& ctx) {
    if (!ctx.spcout.requested) return true;

    const int spctyp = ctx.rho.spctyp;   // 0 = arspec (spgrh), 1 = periodogram
    const int mxarsp = ctx.rho.mxarsp;

    const int sp = ctx.model.sp;
    // The oracle only computes the spectrum for monthly data (x11ari.f:282 gates
    // the spcdrv call on Ny==12); quarterly/other periods produce no spectrum
    // tables. (mkfreq's sp!=12 grid + its quarterly TD-frequency substitutions
    // are stubbed -- a later increment.)
    if (sp != 12) return true;

    const int muladd = ctx.x11opt.muladd;
    const bool lmodel = ctx.captured.has_model;
    const bool psuadd = ctx.x11msc.psuadd;
    const bool ldecbl = ctx.rho.ldecbl;
    const int spcsrs = ctx.rho.spcsrs;

    const int pos1bk = ctx.x11ptr.pos1bk;
    const int pos1ob = ctx.x11ptr.pos1ob;
    const int posfob = ctx.x11ptr.posfob;
    const int posffc = ctx.x11ptr.posffc;

    // Resolve Spdfor (spcdrv.f:106-132): default differencing order.
    int spdfor = ctx.rho.spdfor;
    bool spcdff = ctx.rho.spcdff;
    if (spcdff && spdfor == prm::NOTSET)
        spdfor = lmodel ? std::max(ctx.model.nnsedf + ctx.model.nseadf - 1, 1) : 1;
    if (spdfor == 0) spcdff = false;

    // Bgspec (gtspec.f:324-327 default): eight years back from Endspn, clamped
    // to the series start. Begbk2 (backcast-extended begin) is not tracked as a
    // date in this port; derive it from begspn + (pos1bk - pos1ob).
    const int* begspn = ctx.mdldat.begspn.data();
    // Series span end (ctx.arima.endspn is not populated on this path): the last
    // observed date = begspn + (nspobs - 1).
    int endspn[2];
    addate(begspn, sp, ctx.mdldat.nspobs - 1, endspn);
    int bgspec[2];
    if (ctx.rho.bgspec(1) == prm::NOTSET) {
        addate(endspn, sp, -95, bgspec);
        int nspec = 0;
        dfdate(bgspec, begspn, sp, nspec);
        if (nspec < 0) { bgspec[0] = begspn[0]; bgspec[1] = begspn[1]; }
    } else {
        bgspec[0] = ctx.rho.bgspec(1);
        bgspec[1] = ctx.rho.bgspec(2);
    }
    int begbk2[2];
    addate(begspn, sp, pos1bk - pos1ob, begbk2);
    // spcrsd (spr) runs in the regARIMA phase, before spcdrv applies any Lstdff
    // shift to Bgspec -- snapshot the default Bgspec for the residual span.
    const int bgspec_rsd[2] = {bgspec[0], bgspec[1]};

    // Relative starting position for the spectrum span (spcdrv.f:137-151).
    int ipos = 0;
    dfdate(bgspec, begbk2, sp, ipos);
    ipos += 1;
    int l0, l1;
    if (ctx.rho.lstdff) {
        // startdiff=yes: start the plot at Bgspec and difference the leading
        // Spdfor points; if that would run before Pos1ob, shift Bgspec forward.
        l1 = ipos;
        l0 = l1 - spdfor;
        if (l0 < pos1ob) {
            const int ldsp = pos1ob - l0;
            l1 += ldsp;
            addate(bgspec, sp, ldsp, bgspec);
            l0 = pos1ob;
        }
    } else {
        l0 = ipos;
        l1 = ipos + spdfor;
    }

    const bool taklog = (muladd != 1);
    const bool ltk120 = ctx.rho.ltk120;

    auto& out = ctx.spcout;
    out.frq = mkfreq(sp, /*peakwd=*/1);
    std::vector<double> srs(PLEN, 0.0), tmp(PLEN, 0.0);

    // Estimator selector (spcdrv.f: IF Spctyp==0 spgrh ELSE spgrh2): the AR
    // spectrum for type=arspec, the periodogram for type=periodogram. Returns
    // whether a table was produced.
    auto spec_est = [&](const double* series, int n1, int n2,
                        std::vector<double>& sxx) -> bool {
        if (spctyp == 0)
            return spgrh(series, out.frq, n1, n2, 61, sp, mxarsp, ldecbl, sxx);
        spgrh2(series, out.frq, n1, n2, ldecbl, sxx);
        return true;
    };

    // --- sp0: detrended original / AdjOri (spcdrv.f:161-218) ---------------
    // Increment 1 implements only the Spcsrs>=2 default (adjoriginal/b1):
    // srs = Stcsi, then the extreme-value fold (addmul Stex) for the Lx11
    // Spcsrs==2 non-pseudo-additive case. The Spcsrs<2 branch (series=original/
    // a1 -> Series with Adj{ls,ao,tc,so} divided out, spcdrv.f:178-185) and the
    // Psuadd branch (spcdrv.f:166-174) are follow-on; no corpus spec exercises
    // them (the default spectrumseries is adjoriginal, Spcsrs==2).
    {
        const double* stcsi = ctx.orisrs.stcsi.data();
        for (int i = 1; i <= posfob; ++i) srs[i - 1] = stcsi[i - 1];
        if (spcsrs == 2 && !psuadd)  // Lx11 path, Spcsrs==2
            addmul(srs.data(), srs.data(), ctx.mq10_stex.data(), pos1bk, posffc,
                   muladd);
        gendff(srs.data(), l0, posfob, tmp.data(), taklog, spdfor);
        out.have_sp0 = spec_est(tmp.data(), l1, posfob, out.sp0);
        // st0: Tukey spectrum of the same detrended series (spcdrv.f:250-255).
        out.have_st0 = tukey_spectrum(tmp.data(), l1, posfob, ltk120, out.st0,
                                      out.frq_tukey);
    }
    // --- sp1: detrended seasonally adjusted (spcdrv.f:301-349) -------------
    // The SA series is E2 (Stcime, the SA modified for extreme values), NOT the
    // D11 save: x11pt4 runs before spcdrv, and spcdrv's Stci holds Part-E's E2.
    {
        const double* stcime = ctx.adxser.stcime.data();
        for (int i = 1; i <= posfob; ++i) srs[i - 1] = stcime[i - 1];
        gendff(srs.data(), l0, posfob, tmp.data(), taklog, spdfor);
        out.have_sp1 = spec_est(tmp.data(), l1, posfob, out.sp1);
        // st1: Tukey spectrum of the same detrended SA series (spcdrv.f:388-392).
        out.have_st1 = tukey_spectrum(tmp.data(), l1, posfob, ltk120, out.st1,
                                      out.frq_tukey);
    }
    // --- sp2: irregular (spcdrv.f:438-467) -- no differencing -------------
    // Likewise the irregular is E3 (Stime, the modified irregular), not D13.
    {
        const double* stime = ctx.mq5a_stime.data();
        for (int i = ipos; i <= posfob; ++i) {
            tmp[i - 1] = stime[i - 1];
            if (muladd != 1) tmp[i - 1] -= 1.0;
        }
        out.have_sp2 = spec_est(tmp.data(), ipos, posfob, out.sp2);
        // st2: Tukey spectrum of the same irregular series (spcdrv.f:506-510).
        out.have_st2 = tukey_spectrum(tmp.data(), ipos, posfob, ltk120, out.st2,
                                      out.frq_tukey);
    }
    // --- spr: regARIMA model residuals (spcrsd.f, periodogram path) --------
    // No detrend, no log: the residuals `a` are used directly. Their start date
    // Begrsd = Begspn + (Nspobs - na); the span is [rpos, na] where rpos =
    // dfdate(Bgspec, Begrsd)+1 (clamped to 1 if Bgspec precedes the residuals).
    if (ctx.resid_na > 0) {
        const int na = ctx.resid_na;
        int begrsd[2];
        addate(begspn, sp, ctx.mdldat.nspobs - na, begrsd);
        int rpos = 0;
        dfdate(bgspec_rsd, begrsd, sp, rpos);
        if (rpos < 0) rpos = 1;
        else rpos += 1;
        std::vector<double> ra(PLEN, 0.0);
        for (int i = 1; i <= na; ++i) ra[i - 1] = ctx.resid_a[i - 1];
        out.have_spr = spec_est(ra.data(), rpos, na, out.spr);
    }

    out.ran = true;
    return true;
}

}  // namespace x13
