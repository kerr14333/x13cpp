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

}  // namespace

bool run_spectrum(X13Context& ctx) {
    if (!ctx.spcout.requested) return true;

    // Increment 1 covers the periodogram type only. gt_spectrum sets Spctyp
    // (0=arspec, 1=periodogram); defer arspec to a later increment.
    if (ctx.rho.spctyp != 1) return true;

    const int sp = ctx.model.sp;
    // The oracle only computes the periodogram spectrum for monthly data
    // (x11ari.f:282 gates the spcdrv call on Ny==12); quarterly/other periods
    // produce no spectrum tables. (mkfreq's sp!=12 grid + the arspec type are a
    // later increment; the quarterly TD-frequency substitutions here are stubbed.)
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

    auto& out = ctx.spcout;
    out.frq = mkfreq(sp, /*peakwd=*/1);
    std::vector<double> srs(PLEN, 0.0), tmp(PLEN, 0.0);

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
        spgrh2(tmp.data(), out.frq, l1, posfob, ldecbl, out.sp0);
        out.have_sp0 = true;
    }
    // --- sp1: detrended seasonally adjusted (spcdrv.f:301-349) -------------
    // The SA series is E2 (Stcime, the SA modified for extreme values), NOT the
    // D11 save: x11pt4 runs before spcdrv, and spcdrv's Stci holds Part-E's E2.
    {
        const double* stcime = ctx.adxser.stcime.data();
        for (int i = 1; i <= posfob; ++i) srs[i - 1] = stcime[i - 1];
        gendff(srs.data(), l0, posfob, tmp.data(), taklog, spdfor);
        spgrh2(tmp.data(), out.frq, l1, posfob, ldecbl, out.sp1);
        out.have_sp1 = true;
    }
    // --- sp2: irregular (spcdrv.f:438-467) -- no differencing -------------
    // Likewise the irregular is E3 (Stime, the modified irregular), not D13.
    {
        const double* stime = ctx.mq5a_stime.data();
        for (int i = ipos; i <= posfob; ++i) {
            tmp[i - 1] = stime[i - 1];
            if (muladd != 1) tmp[i - 1] -= 1.0;
        }
        spgrh2(tmp.data(), out.frq, ipos, posfob, ldecbl, out.sp2);
        out.have_sp2 = true;
    }

    out.ran = true;
    return true;
}

}  // namespace x13
