// run_spectrum.cpp -- the spectrum diagnostics (spcdrv.f / spcrsd.f).
//
// The four data-based spectra sp0/sp1/sp2/spr, their Tukey twins st0/st1/st2,
// and the PEAK savelog block (spectrum_peaks.cpp). The frequency grid is
// mkfreq.f; the detrending is gendff.f (log + regular differencing); the
// estimator is spgrh.f (arspec, the default) or spgrh2.f (periodogram).
//
// This is NOT gated on a `spectrum{}` spec: x11ari.f:282-287 calls spcdrv under
// a plain IF(Ny.eq.12), so the oracle computes the whole block on every monthly
// run. getTPeaks' Tukey peak probabilities (specpeak.f Tpeaks2, in
// spectrum_peaks.cpp), genqs, the model-only path and the SEATS branch
// (spcdrv's Hvstsa/Hvstir arms) are all closed. Still follow-on: the plots and
// warnings, the Iagr>3 indirect names, and `spectrum{altfreq=}` (CB-30).
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
std::vector<double> mkfreq(int sp, int peakwd, bool lfqalt, bool lprsfq) {
    std::vector<double> frq(61);
    for (int i = 0; i < 61; ++i) frq[i] = static_cast<double>(i) / 120.0;
    const double f2 = frq[1];  // Frq(2) == 1/120
    if (!(sp == 12)) return frq;  // quarterly TD grid handled in a later increment
    // mkfreq.f:54-70 -- the whole substitution block is skipped when
    // `showseasonalfreq` (Lprsfq) is on: the caller then wants the plain
    // seasonal grid and none of the trading-day frequencies spliced into it.
    if (lprsfq) return frq;
    // `altfreq` (Lfqalt) adds a THIRD trading-day frequency at .3036, and note
    // its upper limit is guarded on `Peakwd < 4` where the other two are not
    // (mkfreq.f:57-59) -- transcribed, not tidied.
    if (lfqalt) {
        frq[(37 - peakwd) - 1] = 0.3036 - f2 * peakwd;
        frq[37 - 1] = 0.3036;
        if (peakwd < 4) frq[(37 + peakwd) - 1] = 0.3036 + f2 * peakwd;
    }
    // Frq(43-Peakwd) .. Frq(43+Peakwd) around .3482, Frq(53..) around .432.
    // 1-based Fortran indices -> subtract 1 for the 0-based vector. The
    // `Peakwd != 2` guard on the LOWER limit only is likewise verbatim.
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
//
// Split into the FIT and the EVALUATION because the peak diagnostics need the
// same estimator on a second, enhanced frequency grid: the fit is the expensive
// half (O(n*lagh1) autocovariance + O(ifpl^2) Levinson-Durbin) and depends only
// on the series, so evaluating twice off one fit is free where re-fitting would
// have roughly doubled the suite's runtime. Bit-identical either way.
struct ArFit {
    bool ok = false;
    std::vector<double> coef;
    int l = 0;
    double sgme2 = 0.0;
};

// `nspfrq` sets sautco's lag truncation (spgrh.f passes the frequency count),
// so it belongs to the FIT even though it names a grid length.
ArFit spgrh_fit(const double* yy, int n1, int n2, int nspfrq, int sp,
                int mxarsp) {
    ArFit f;
    const int n = n2 - n1 + 1;
    std::vector<double> x(PLEN, 0.0);
    for (int i = n1; i <= n2; ++i) x[i - 1] = yy[i - 1];
    const int h = nspfrq - 1;
    const int lagh1 = std::min(n - 1, h) + 1;
    std::vector<double> cxx;
    if (!sautco(x.data(), n1, n2, n, lagh1, cxx)) return f;
    int ifpl = (mxarsp == prm::NOTSET) ? 30 * sp / 12 : mxarsp;
    ifpl = std::min(ifpl, n - 1);
    sicp2(cxx, ifpl + 1, n, f.coef, f.l, f.sgme2);
    f.ok = true;
    return f;
}

void spgrh_eval(const ArFit& f, const std::vector<double>& frq, int nspfrq,
                bool ldecbl, std::vector<double>& sxx) {
    const double PI = 3.14159265358979;
    sxx.assign(static_cast<std::size_t>(nspfrq), 0.0);
    for (int i = 0; i < nspfrq; ++i) {
        double c2 = 1.0, s2 = 0.0;
        for (int k = 1; k <= f.l; ++k)
            c2 += f.coef[k] * std::cos(2.0 * k * PI * frq[i]);
        for (int k = 1; k <= f.l; ++k)
            s2 += f.coef[k] * std::sin(2.0 * k * PI * frq[i]);
        double pxx = f.sgme2 / (c2 * c2 + s2 * s2);
        if (ldecbl) { if (pxx < 0.0) pxx = -pxx; pxx = 10.0 * std::log10(pxx); }
        sxx[i] = pxx;
    }
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
                    std::vector<double>& st, std::vector<double>& frq,
                    TukeyPeaks* peaks = nullptr, int mq = 12, int min_nz = 81) {
    const int nz = n2 - n1 + 1;
    // spcdrv.f:252 gates on `nsrs.gt.80` and spcrsd.f:112 on `ntmp.ge.80` -- a
    // one-observation difference between two call sites of the same routine,
    // so the bound is a parameter rather than baked in.
    if (nz < min_nz) return false;
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
    // Tpeaks2 works on the RAW spectrum, not the decibel one savstp punches --
    // its statistics are ratios of neighbouring ordinates scored against an F
    // distribution, which a log would turn into differences.
    if (peaks) *peaks = tpeaks2(p.data(), m, mq, nz);
    return true;
}

}  // namespace

bool run_spectrum(X13Context& ctx, bool iagr4) {
    // NO gate on ctx.spcout.requested: x11ari.f:282-287 calls spcdrv under a
    // plain IF(Ny.eq.12), with no dependence on the spectrum{} spec at all, so
    // the oracle computes this block on every monthly run -- 289 of the 331
    // `.udg` goldens carry `spcori.*`. The save TABLES stay keyed on the request
    // (the harness only emits them when asked); the savelog canaries do not.
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

    // Bgspec is resolved at the parse tail (gtinpt.f:1282-1286 / gtspec.f:324-
    // 327), because the residual QS statistic reads it during estimation.
    // Begbk2 (the backcast-extended begin) is not tracked as a date in this
    // port; derive it from begspn + (pos1bk - pos1ob).
    const int* begspn = ctx.mdldat.begspn.data();
    int bgspec[2] = {ctx.rho.bgspec(1), ctx.rho.bgspec(2)};
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

    // spcdrv.f:193-200 -- the detrend takes logs on a DIFFERENT test depending
    // on whether this is an adjustment run. With Lx11 it is the X-11 mode
    // (Muladd != 1); without one there is no mode to read, so it keys on the
    // TRANSFORM instead (Lam == 0, i.e. transform{function=log}). The two agree
    // for a log X-11 run and disagree for every non-log model-only spec, where
    // Muladd still carries its multiplicative default: measured `spcori.median`
    // +24.29 (oracle) against -27.79 (engine, logging a sqrt-transformed
    // series) on generated/airline_trans-sqrt.
    const bool taklog = ctx.captured.has_x11 ? (muladd != 1)
                                             : (ctx.arima.lam == 0.0);
    const bool ltk120 = ctx.rho.ltk120;

    auto& out = ctx.spcout;
    out.frq = mkfreq(sp, ctx.rho.peakwd, ctx.rho.lfqalt, ctx.rho.lprsfq);
    std::vector<double> srs(PLEN, 0.0), tmp(PLEN, 0.0);

    // mkpeak.f + mkfreq.f -- the constant peak-index tables and the ENHANCED
    // frequency grid the peak tests index. Built once; ok=false for any
    // configuration this increment declines (see spectrum_peaks.cpp).
    const SpecPeakGrid pkgrid = spectrum_peak_grid(sp, ctx.rho.peakwd,
                                                   ctx.rho.lfqalt,
                                                   ctx.rho.lprsfq);
    // x11ari.f:344's second spcdrv APPENDS: savpk.f splits the accumulated
    // peak strings at the direct pass's own count, so the direct entries have to
    // survive. Its table/peak results go to the `*_ind` fields instead.
    if (!iagr4) out.peaks.clear();
    out.peaks_ind.clear();
    out.tukey_ind.clear();
    out.grid = pkgrid;
    std::vector<SpecPeaks>& peaks_sink = iagr4 ? out.peaks_ind : out.peaks;
    std::vector<TukeyEntry>& tukey_sink = iagr4 ? out.tukey_ind : out.tukey;

    // Estimator selector (spcdrv.f: IF Spctyp==0 spgrh ELSE spgrh2): the AR
    // spectrum for type=arspec, the periodogram for type=periodogram. Fills the
    // 61-point plot grid AND the enhanced peak grid from one fit, then runs the
    // peak diagnostics on the pair (svpeak.f reads both). Returns whether a
    // table was produced.
    auto spec_est = [&](const double* series, int n1, int n2, bool ltdfrq,
                        const char* prefix, std::vector<double>& sxx) -> bool {
        std::vector<double> sxx2;
        if (spctyp == 0) {
            const ArFit f = spgrh_fit(series, n1, n2, 61, sp, mxarsp);
            if (!f.ok) return false;
            spgrh_eval(f, out.frq, 61, ldecbl, sxx);
            if (pkgrid.ok)
                spgrh_eval(f, pkgrid.frqpk, pkgrid.nfreq, ldecbl, sxx2);
        } else {
            spgrh2(series, out.frq, n1, n2, ldecbl, sxx);
            if (pkgrid.ok) spgrh2(series, pkgrid.frqpk, n1, n2, ldecbl, sxx2);
        }
        if (pkgrid.ok)
            peaks_sink.push_back(spectrum_peaks(sxx, sxx2, pkgrid, ctx.rho.spclim,
                                                ldecbl, ltdfrq, ctx.rho.plocal,
                                                sp, prefix));
        return true;
    };
    // spcdrv.f:152-153 -- the trading-day frequencies are only searched when the
    // spectrum span is longer than NTDLIM=60 observations.
    const bool ltdfrq_main = (posfob - l1 + 1) > 60;

    const bool lx11 = ctx.captured.has_x11;
    const int kfulsm = ctx.x11opt.kfulsm;
    const bool lrbstsa = ctx.rho.lrbstsa;
    // ispos.f -- every observation over [n1,n2] strictly positive. The oracle
    // refuses to take the LOG of a series that is not (Muladd==0 is the
    // multiplicative mode, where gendff logs), and simply produces no table.
    auto ispos = [](const double* v, int n1, int n2) {
        for (int i = n1; i <= n2; ++i)
            if (!(v[i - 1] > 0.0)) return false;
        return true;
    };

    // --- sp0: detrended original / AdjOri (spcdrv.f:161-218) ---------------
    // Increment 1 implements the Spcsrs>=2 default (adjoriginal/b1): srs =
    // Stcsi, then either the pseudo-additive rebuild or the extreme-value fold
    // (addmul Stex) on the Lx11 Spcsrs==2 path. The Spcsrs<2 branch
    // (series=original/a1 -> Series with Adj{ls,ao,tc,so} divided out,
    // spcdrv.f:178-185) is follow-on; no corpus spec exercises it (the default
    // spectrumseries is adjoriginal, Spcsrs==2).
    // spcdrv.f:158 -- `goori = Iagr.le.3`: there is no INDIRECT original, and
    // mkspky.f:14 would name it `spccomp` rather than `spcori` in any case.
    if (!iagr4) {
        const double* stcsi = ctx.orisrs.stcsi.data();
        for (int i = 1; i <= posfob; ++i) srs[i - 1] = stcsi[i - 1];
        if (lx11 && spcsrs == 2) {
            if (psuadd) {
                // spcdrv.f:166-174 -- under pseudo-additive the "original" is
                // REBUILT from the components rather than folded, because the
                // pseudo-additive irregular is centred on one and Stcsi is not
                // the product it is elsewhere.
                // Sti/Stc via the live accessors on principle -- spcdrv runs
                // after x11pt3, where the oracle still sees the INTERNAL
                // values. Here it is provably INERT, and the proof is worth
                // keeping: editor.f:2508-2523 refuses a pseudo-additive run
                // outright when any of Adjao/Adjtc/Adjls/Finao/Finls/Fintc is
                // set or when Nustad>0, and those are exactly the disjuncts of
                // have_sti2/have_stc2 in x11pt3 -- so under Psuadd the mirror
                // IS the internal value and nothing here can diverge. (Measured
                // twice: the oracle rejects `mode=pseudoadd` both with outlier
                // regressors and with a temporary trend prior.) Sts is never
                // published over and is read directly.
                const double* stc = ctx.stc_live();
                const double* sts = ctx.x11srs.sts.data();
                const double* sti = ctx.sti_live();
                for (int i = pos1ob; i <= posfob; ++i) {
                    if (kfulsm == 2)
                        srs[i - 1] = stc[i - 1] * sti[i - 1];
                    else
                        srs[i - 1] = stc[i - 1] * (sts[i - 1] + (sti[i - 1] - 1.0));
                }
            } else {
                addmul(srs.data(), srs.data(), ctx.mq10_stex.data(), pos1bk,
                       posffc, muladd);
            }
        }
        bool goori = true;
        if (muladd == 0) goori = ispos(srs.data(), ipos, posfob);
        if (goori) {
            gendff(srs.data(), l0, posfob, tmp.data(), taklog, spdfor);
            out.have_sp0 = spec_est(tmp.data(), l1, posfob, ltdfrq_main,
                                    "spcori", out.sp0);
            // st0: Tukey spectrum of the same series (spcdrv.f:250-255).
            TukeyPeaks tpk;
            out.have_st0 = tukey_spectrum(tmp.data(), l1, posfob, ltk120,
                                          out.st0, out.frq_tukey, &tpk, sp);
            if (out.have_st0) out.tukey.push_back({"ori", tpk});
        }
    }
    // --- sp1/sp2: only when an adjustment was actually produced -------------
    // spcdrv.f:284 and :406 -- `(Lx11.and.Kfulsm.eq.0).or.Lseats`. Kfulsm is
    // x11{type=}: `summary` (1) and `trend` (2) produce no seasonally adjusted
    // series to take a spectrum OF, and the oracle emits neither table nor
    // peak block for them.
    if ((lx11 && kfulsm == 0) || ctx.captured.has_seats) {
        // --- sp1: detrended seasonally adjusted (spcdrv.f:301-349) ---------
        // The SA series is E2 (Stcime, the SA modified for extreme values), NOT
        // the D11 save: x11pt4 runs before spcdrv, and spcdrv's Stci holds
        // Part-E's E2. `spectrumrobustsa=no` (Lrbstsa false) takes Stci instead.
        // spcdrv.f:322-327 -- the SEATS arm takes the pair run_seats publishes
        // (Stocsa / Seatsa) instead, and note it gets NEITHER the Facls divide
        // below (that sits in the Iagr==4 and Lx11 arms only) NOR the ispos
        // refusal (guarded on Lx11). Its own gate is Hvstsa, spcdrv.f:299.
        const double* sa = ctx.captured.has_seats
                               ? (lrbstsa ? ctx.seatcm.stocsa.data()
                                          : ctx.seatcm.seatsa.data())
                               : (lrbstsa ? ctx.adxser.stcime.data()
                                          : ctx.x11srs.stci.data());
        for (int i = 1; i <= posfob; ++i) srs[i - 1] = sa[i - 1];
        // spcdrv.f:318 -- the LEVEL SHIFT is taken back out of the SA series
        // before its spectrum, on the Lrbstsa (default) path only. Note this
        // divide is unconditional on `Finls`, unlike x11pt4's E2 block, and it
        // runs over [ipos,Posfob] rather than the full padded range. Without it
        // every spec carrying an LS regressor -- explicit or automatically
        // identified -- gets a different SA spectrum (measured: spcsa.range
        // 12.418 vs 12.608 on generated/airline_regb-initial).
        if (lrbstsa && ctx.x11adj.adjls == 1)
            divsub(srs.data(), srs.data(), ctx.x11fac.facls.data(), ipos,
                   posfob, muladd);
        // spcdrv.f:290-298 -- the same positivity refusal as the original, but
        // only on the Lx11 + Lrbstsa path.
        bool gosa = lx11 || !ctx.captured.has_seats || ctx.seatlg.hvstsa;
        if (lx11 && lrbstsa && muladd == 0)
            gosa = ispos(srs.data(), ipos, posfob);
        if (gosa) {
            gendff(srs.data(), l0, posfob, tmp.data(), taklog, spdfor);
            bool& hsp1 = iagr4 ? out.have_sp1_ind : out.have_sp1;
            hsp1 = spec_est(tmp.data(), l1, posfob, ltdfrq_main,
                            iagr4 ? "spcindsa" : "spcsa",
                            iagr4 ? out.sp1_ind : out.sp1);
            // st1: Tukey spectrum of the same series (spcdrv.f:388-392).
            TukeyPeaks tpk;
            bool& hst1 = iagr4 ? out.have_st1_ind : out.have_st1;
            hst1 = tukey_spectrum(tmp.data(), l1, posfob, ltk120,
                                  iagr4 ? out.st1_ind : out.st1,
                                  out.frq_tukey, &tpk, sp);
            // svtukp.f:47-56 -- LSPT1I labels the entry `indsa`, not `sa`.
            if (hst1) tukey_sink.push_back({iagr4 ? "indsa" : "sa", tpk});
        }
        // --- sp2: irregular (spcdrv.f:438-467) -- no differencing ----------
        // Likewise the irregular is E3 (Stime, the modified irregular), not D13.
        // Same live-accessor requirement as the pseudo-additive rebuild above:
        // spcdrv.f:444's Sti is the internal D13. Measured on an airline spec
        // with ao/tc/ls regressors and `spectrum{robustsa=no}`: spcirr.median
        // -44.497 off the published D13 against the oracle's -37.352.
        // spcdrv.f:446-451 -- the SEATS pair again, and again with its own gate
        // (`goirr = Hvstir`, spcdrv.f:437). The `-1` re-centring below is shared:
        // Stocir is the percent-scale irregular and Seatir the /100 ratio, so
        // the two arms are the same series at different scales, exactly as in
        // genqs (see the header of core/src/diag/genqs.cpp).
        const double* ir = ctx.captured.has_seats
                               ? (lrbstsa ? ctx.seatcm.stocir.data()
                                          : ctx.seatcm.seatir.data())
                               : (lrbstsa ? ctx.mq5a_stime.data()
                                          : ctx.sti_live());
        const bool goirr = lx11 || !ctx.captured.has_seats || ctx.seatlg.hvstir;
        if (goirr) {
            for (int i = ipos; i <= posfob; ++i) {
                tmp[i - 1] = ir[i - 1];
                if (muladd != 1) tmp[i - 1] -= 1.0;
            }
            bool& hsp2 = iagr4 ? out.have_sp2_ind : out.have_sp2;
            hsp2 = spec_est(tmp.data(), ipos, posfob, ltdfrq_main,
                            iagr4 ? "spcindirr" : "spcirr",
                            iagr4 ? out.sp2_ind : out.sp2);
            // st2: Tukey spectrum of the same irregular (spcdrv.f:506-510).
            TukeyPeaks tpk;
            bool& hst2 = iagr4 ? out.have_st2_ind : out.have_st2;
            hst2 = tukey_spectrum(tmp.data(), ipos, posfob, ltk120,
                                  iagr4 ? out.st2_ind : out.st2,
                                  out.frq_tukey, &tpk, sp);
            if (hst2) tukey_sink.push_back({iagr4 ? "indirr" : "irr", tpk});
        }
    }
    // --- spr: regARIMA model residuals (spcrsd.f, periodogram path) --------
    // No detrend, no log: the residuals `a` are used directly. Their start date
    // Begrsd = Begspn + (Nspobs - na); the span is [rpos, na] where rpos =
    // dfdate(Bgspec, Begrsd)+1 (clamped to 1 if Bgspec precedes the residuals).
    // The residual block belongs to the regARIMA phase, so the INDIRECT pass
    // (which runs after agr3, on a total that has no model of its own) never
    // re-derives it -- spcrsd is called from arima.f:1126, not from spcdrv.
    if (!iagr4 && ctx.resid_na > 0) {
        const int na = ctx.resid_na;
        // arima.f:1125's idate, recorded at estimation time (see ctx.resid_begdate).
        const int begrsd[2] = {ctx.resid_begdate[0], ctx.resid_begdate[1]};
        int rpos = 0;
        dfdate(bgspec_rsd, begrsd, sp, rpos);
        if (rpos < 0) rpos = 1;
        else rpos += 1;
        std::vector<double> ra(PLEN, 0.0);
        for (int i = 1; i <= na; ++i) ra[i - 1] = ctx.resid_a[i - 1];
        // spcrsd.f:74 derives its OWN Ltdfrq from the residual span.
        out.have_spr = spec_est(ra.data(), rpos, na, (na - rpos + 1) > 60,
                                "spcrsd", out.spr);
        // spcrsd.f:110-119 -- the residual Tukey peaks, and its own gates: the
        // span must be at least 80 long (`ntmp.ge.80`, note `>=` where
        // spcdrv.f:252 uses a strict `>`) and Sp must be 12.
        //
        // CB-28, transcribed: the `IF(ipos.gt.1)` block copies a(ipos..na) into
        // Temp and the call on the very next line passes `a`, not `Temp` -- so
        // the shift to the diagnostic start date is computed and thrown away,
        // and the residual Tukey spectrum is always taken from element 1. Only
        // the LENGTH (ntmp) reflects ipos. spcdrv's three call sites do the
        // same repack correctly, which is what makes this one a slip rather
        // than a convention.
        const int ntmp = na - rpos + 1;
        if (ntmp >= 80 && sp == 12) {
            TukeyPeaks tpk;
            std::vector<double> rst, rfrq;
            if (tukey_spectrum(ra.data(), 1, ntmp, ltk120, rst, rfrq, &tpk, sp,
                               /*min_nz=*/80))
                out.tukey.insert(out.tukey.begin(), {"rsd", tpk});
        }
    }

    // --- savpk.f: the accumulated peak-label lists -------------------------
    // spcdrv.f:750-794 appends one label per table that found a peak, in the
    // order rsd (spcrsd.f:213-224, run in the regARIMA phase) then ori, sa,
    // irr. The SEASONAL list only takes `ori` when NO seasonal adjustment was
    // done (spcdrv.f:755 `nosa`) -- searching the original for a seasonal peak
    // is only meaningful then. savpk.f turns an empty list into "none".
    {
        const bool nosa = !((lx11 && kfulsm == 0) || ctx.captured.has_seats);
        std::string cs, ct;
        auto add = [](std::string& s, const char* lab) {
            if (!s.empty()) s += ' ';
            s += lab;
        };
        auto find = [&](const char* prefix) -> const SpecPeaks* {
            for (const auto& p : out.peaks)
                if (p.prefix == prefix) return &p;
            for (const auto& p : out.peaks_ind)
                if (p.prefix == prefix) return &p;
            return nullptr;
        };
        // x11ari.f:290-291 records the DIRECT pass's counts (`nspdir`/`ntpdir`)
        // between the two spcdrv calls, and savpk.f:85-116 splits the ACCUMULATED
        // string there: characters 1..nspdir are the direct peaks and the rest
        // the indirect ones. Tracked here as label counts rather than character
        // offsets, which is equivalent and does not depend on label widths.
        std::size_t nsdir = 0, ntdir = 0;
        if (const SpecPeaks* p = find("spcrsd")) {
            // Always `rsd`, on a SEATS run too. spcrsd.f:209-215 picks
            // `extrsd` off its own `Lseats` ARGUMENT, not off the run's, and
            // there are two call sites: arima.f:1126 passes F (these are the
            // regARIMA residuals, which is what this block IS) and seatpr.f:145
            // passes T for the SEATS EXTENDED residuals `Srsdex`, a different
            // input entirely. That second one is unported and correctly
            // untested -- seatpr.f:142 gates it on Prttab/Savtab(LSPERS), NOT
            // on Lsumm, so the `-s` flag alone never produces it and no corpus
            // golden carries a single `spcextrsd` key.
            if (p->ltdpk > 0) add(ct, "rsd");
            if (p->lsapk > 0) add(cs, "rsd");
        }
        if (const SpecPeaks* p = find("spcori")) {
            if (p->ltdpk > 0) add(ct, "ori");
            if (nosa && p->lsapk > 0) add(cs, "ori");
        }
        if (const SpecPeaks* p = find("spcsa")) {
            if (p->ltdpk > 0) add(ct, "sa");
            if (p->lsapk > 0) add(cs, "sa");
        }
        if (const SpecPeaks* p = find("spcirr")) {
            if (p->ltdpk > 0) add(ct, "irr");
            if (p->lsapk > 0) add(cs, "irr");
        }
        // Everything above belongs to the DIRECT pass; count it before the
        // indirect entries are appended.
        auto nlab = [](const std::string& s) {
            if (s.empty()) return std::size_t{0};
            std::size_t n = 1;
            for (char c : s) if (c == ' ') ++n;
            return n;
        };
        nsdir = nlab(cs);
        ntdir = nlab(ct);
        if (const SpecPeaks* p = find("spcindsa")) {
            if (p->ltdpk > 0) add(ct, "indsa");
            if (p->lsapk > 0) add(cs, "indsa");
        }
        if (const SpecPeaks* p = find("spcindirr")) {
            if (p->ltdpk > 0) add(ct, "indirr");
            if (p->lsapk > 0) add(cs, "indirr");
        }
        out.peaks_seas = cs.empty() ? "none" : cs;
        out.peaks_td = ct.empty() ? "none" : ct;

        // savpk.f:88-115 -- the `.dir` / `.ind` split, emitted only on a
        // composite total (`Iagr.gt.3`). The three degenerate branches are NOT
        // the same as splitting an empty list: with no peak anywhere BOTH sides
        // print the whole (i.e. "none") string, with nothing direct the INDIRECT
        // side gets the whole string and the direct side a literal "none", and
        // with everything direct it is the other way round.
        //
        // UNGATED, and measured to be so: the only composite corpus finds NO
        // visually significant peak in any table, so all four keys are "none"
        // and only the first branch is ever taken. A mutation swapping the two
        // output halves passes the whole suite. Gating the real split needs a
        // composite whose components carry a residual seasonal or trading-day
        // peak; until then this transcription is unverified past the degenerate
        // case.
        if (iagr4) {
            auto split = [&](const std::string& all, std::size_t ndir,
                             std::string& dir, std::string& ind) {
                const std::size_t n = nlab(all);
                if (all.empty() || all == "none") { dir = "none"; ind = "none"; return; }
                if (ndir == 0) { dir = "none"; ind = all; return; }
                if (ndir == n) { dir = all; ind = "none"; return; }
                std::size_t cut = 0, seen = 0;
                for (std::size_t i = 0; i < all.size(); ++i) {
                    if (all[i] == ' ' && ++seen == ndir) { cut = i; break; }
                }
                dir = all.substr(0, cut);
                ind = all.substr(cut + 1);
            };
            split(out.peaks_seas, nsdir, out.peaks_seas_dir, out.peaks_seas_ind);
            split(out.peaks_td, ntdir, out.peaks_td_dir, out.peaks_td_ind);
        }
    }

    // svtukp.f -- the `peaks.tukey.*` lists, over the Itukey entries above.
    // x11ari.f:76's lsadj is `Lx11.or.Lseats`, i.e. "this run produced an
    // adjustment"; it is the only thing that makes svtukp's oriIdx (CB-28's
    // sibling, see spectrum_peaks.cpp) anything but NOTSET.
    // x11ari.f:355-357 calls svtukp a second time on the INDIRECT pass, over
    // that pass's own Itukey entries only (Ntukey is reset between the two by
    // spcdrv's own accumulator), so the two label sets are independent -- unlike
    // savpk's, which share one accumulated string and get split.
    if (iagr4) {
        out.tukey_labels_ind =
            tukey_peak_labels(out.tukey_ind, lx11 || ctx.captured.has_seats);
        out.ran_ind = true;
    } else {
        out.tukey_labels =
            tukey_peak_labels(out.tukey, lx11 || ctx.captured.has_seats);
        out.ran = true;
    }
    return true;
}

}  // namespace x13
