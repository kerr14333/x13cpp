// genqs.cpp -- the QS seasonality statistics (genqs.f, 666 lines, plus the
// three leaves calcqs.f / calcqs2.f / qsdiff.f).
//
// 327 of the 331 `.udg` goldens carry a `qs*` key and the port emitted none of
// them. Like the spectrum peak block this is NOT gated on any spec: x11ari.f
// :277-281 runs genqs whenever the LSPCQS table is wanted, and gtinpt.f:122
// makes `-s` (Lsumm>0) set Savtab from `sumtab`, whose entry 113 (LSPCQS) is
// TRUE. Unlike the spectrum block there is no `IF(Ny.eq.12)` in front of it --
// editor.f:855-859's monthly-only Savtab clear covers LSPCS0..LSPS0C (93..102)
// and LSPCTP/LSPCQC (115/116), NOT LSPCQS -- so quarterly specs are in scope.
//
// This increment covers the DIRECT X-11 path (Iagr<4, Lx11). Still open, and
// skipped with the reason written at the gate's skip rather than filtered out
// of discovery: the SEATS branch (Seatsa/Seatir/Stocsa/Stocir + Hvstsa/Hvstir,
// buffers this port's SEATS chain does not fill), the MODEL-ONLY path
// (x12run.f:181 reaches x11ari with neither Lx11 nor Lseats, which the port's
// model-only harness does not run at all), and the Iagr==4 indirect names.
#include "diag/genqs.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "common/x13context.hpp"
#include "diag/checkres.hpp"         // kendalls (shared with npsa.f)
#include "numeric/numeric.hpp"       // smeadl
#include "specparse/specparse.hpp"   // addate, dfdate
#include "x11/x11filt.hpp"           // addmul, divsub

namespace x13 {
namespace {

constexpr int PLEN = 1020;

// x11pt3.f builds the PUBLISHED D13 and D12 in its own `sti2`/`stc2` locals and
// leaves /x11srs/ Sti and Stc INTERNAL -- outlier-removed and level-shift-free
// respectively. This port copies the published values back over Sti/Stc at the
// tail of x11pt3 so the harness can punch d13/d12 straight off the COMMON
// mirror, which means anything reading /x11srs/ AFTER x11pt3 (as genqs and
// x11pt4 both do) has to take the snapshot instead. Measured: on
// generated/expgs_fixed-airline-x11 the published D13 gives qsirr 0.03946
// against the oracle's 0.00000, because folding the AOs back in flips the
// lag-4 autocorrelation positive and calcqs is a step function in its sign.
const double* x11_sti_live(const X13Context& ctx) {
    return ctx.x11_sti_int.empty() ? ctx.x11srs.sti.data()
                                   : ctx.x11_sti_int.data();
}
const double* x11_stc_live(const X13Context& ctx) {
    return ctx.x11_stc_int.empty() ? ctx.x11srs.stc.data()
                                   : ctx.x11_stc_int.data();
}

// calcqs2.f: the QS statistic of z(1..nz), PLUS the `PosCorr` verdict qsdiff
// uses to decide whether to difference once more. Note the two differ in more
// than the extra output -- calcqs2 works on the WHOLE array from element 1,
// divides the autocovariances by `nz` rather than by a separate count, walks
// every lag up to 2*mq (not just mq and 2*mq), and tests `r(mq) > 0` where
// calcqs tests `r(1) > 0`. Transcribed as written.
void calcqs2(const double* z, int nz, int mq, double& qs, int& poscorr) {
    double c0 = 0.0;
    for (int i = 1; i <= nz; ++i) c0 += z[i - 1] * z[i - 1];
    c0 /= nz;
    const int nr = mq + mq;
    std::vector<double> r(static_cast<std::size_t>(nr) + 1, 0.0);
    for (int k = 1; k <= nr; ++k) {
        double c = 0.0;
        for (int i = k + 1; i <= nz; ++i) c += z[i - 1] * z[(i - k) - 1];
        c /= nz;
        r[k] = c / c0;
    }
    poscorr = 1;
    if (mq > 4) {
        if (r[mq] <= 0.0) {
            poscorr = 0;
        } else {
            for (int i = 1; i <= 4; ++i)
                if (r[i] <= 0.0) poscorr = 0;
        }
    } else {
        // NOTE the threshold is 0.2 here and 0.0 above: for a quarterly series
        // the seasonal lag is close enough to the short lags that the oracle
        // demands a materially positive correlation at every one of them.
        for (int i = 1; i <= mq; ++i)
            if (r[i] <= 0.2) poscorr = 0;
    }
    qs = 0.0;
    if (mq != 1 && r[mq] > 0.0) {
        for (int j = 1; j <= 2; ++j) {
            const int k = j * mq;
            if (r[k] > 0.0) qs += (r[k] * r[k]) / (nz - k);
        }
        qs = qs * nz * (nz + 2);
    }
}

}  // namespace

// calcqs.f.
double calcqs(const double* z, int iconce, int nz, int mq) {
    const int nr = nz - iconce;
    double c0 = 0.0;
    for (int i = iconce + 1; i <= nz; ++i) c0 += z[i - 1] * z[i - 1];
    c0 /= nr;
    double r[3] = {0.0, 0.0, 0.0};
    for (int k = 1; k <= 2; ++k) {
        double c = 0.0;
        const int j = k * mq + 1 + iconce;
        for (int i = j; i <= nz; ++i) c += z[i - 1] * z[(i - k * mq) - 1];
        c /= nr;
        r[k] = c / c0;
    }
    double qs = 0.0;
    // The whole statistic is ZERO unless the SEASONAL-lag autocorrelation is
    // positive -- QS tests for positive seasonal autocorrelation specifically,
    // so a negative one is reported as "no evidence" rather than as a large
    // statistic. This is why so many corpus goldens read `0.00000  1.00000`.
    if (mq != 1 && r[1] > 0.0) {
        for (int k = 1; k <= 2; ++k)
            if (r[k] > 0.0) qs += (r[k] * r[k]) / (nr - k * mq);
        qs = qs * nr * (nr + 2);
    }
    return qs;
}

// qsdiff.f.
void qs_diff(const double* srs, int pos1, int posf, bool lmodel, int nnsedf,
             int nseadf, int ny, double& qs) {
    const int nz = posf - pos1 + 1;
    std::vector<double> aux(PLEN, 0.0);
    for (int i = pos1; i <= posf - 1; ++i)
        aux[(i - pos1 + 1) - 1] = srs[(i + 1) - 1] - srs[i - 1];
    int k = nz - 1;
    // With a model the differencing order follows the model's own, capped at 2
    // and floored at 1; without one it is a single difference.
    const int ndif = lmodel ? std::max(std::min(2, nnsedf + nseadf), 1) : 1;
    if (ndif > 1) {
        for (int j = 1; j <= ndif - 1; ++j) {
            --k;
            for (int i = 1; i <= k; ++i) aux[i - 1] = aux[(i + 1) - 1] - aux[i - 1];
        }
    }
    double xmean = 0.0;
    smeadl(aux.data(), 1, k, k, xmean);
    int poscorr = 0;
    calcqs2(aux.data(), k, ny, qs, poscorr);
    // If the once-differenced series still shows positive autocorrelation
    // everywhere calcQS2 looked, the trend has not been removed -- difference
    // again and REPLACE the statistic. Only from ndif==1: a model that already
    // asked for two differences does not get a third.
    if (poscorr == 1 && ndif == 1) {
        --k;
        for (int i = 1; i <= k; ++i) aux[i - 1] = aux[(i + 1) - 1] - aux[i - 1];
        calcqs2(aux.data(), k, ny, qs, poscorr);
    }
}

bool QsStats::lqs() const {
    return !(qsori == prm::DNOTST && qsrsd == prm::DNOTST &&
             qssadj == prm::DNOTST && qsirr == prm::DNOTST &&
             qssadj2 == prm::DNOTST && qsirr2 == prm::DNOTST &&
             qsori2 == prm::DNOTST);
}

bool QsStats::lqss() const {
    return !(qsoris == prm::DNOTST && qsrsd2 == prm::DNOTST &&
             qssadjs == prm::DNOTST && qsirrs == prm::DNOTST &&
             qssadjs2 == prm::DNOTST && qsirrs2 == prm::DNOTST &&
             qsoris2 == prm::DNOTST);
}

// genqs.f:1-254 -- the arithmetic half. The print/savelog halves are the
// harness's (this port writes no files); `lqs`/`lqss` are on QsStats.
bool genqs(X13Context& ctx, bool lseats) {
    const int ny = ctx.model.sp;
    const bool lx11 = ctx.captured.has_x11;
    const int muladd = ctx.x11opt.muladd;
    const int kfulsm = ctx.x11opt.kfulsm;
    const bool psuadd = ctx.x11msc.psuadd;
    const bool lmodel = ctx.captured.has_model;
    const bool llogqs = ctx.rho.llogqs;
    const int pos1bk = ctx.x11ptr.pos1bk;
    const int pos1ob = ctx.x11ptr.pos1ob;
    const int posfob = ctx.x11ptr.posfob;
    const int posffc = ctx.x11ptr.posffc;
    const int nnsedf = ctx.model.nnsedf;
    const int nseadf = ctx.model.nseadf;

    // The SEATS branch reads Seatsa/Seatir/Stocsa/Stocir behind Hvstsa/Hvstir
    // (genqs.f:140/188/230/248) -- COMMONs this port's SEATS chain does not
    // fill, since it publishes its decomposition onto ctx instead. Same blocker
    // the spectrum increment hit: a different INPUT, not just a different
    // driver. Declined here rather than silently reported from the (empty)
    // X-11 buffers.
    if (lseats) return false;

    auto& q = ctx.qs;
    q.ran = true;

    // ipos = dfdate(Bgspec, Begbk2): how far the spectrum/QS start date sits
    // past the backcast-extended series start. Bgspec is resolved once at the
    // parse tail (gtinpt.f:1282-1286 / gtspec.f:324-327); Begbk2 is not tracked
    // as a date in this port, so derive it the way run_spectrum does.
    int begbk2[2];
    addate(ctx.mdldat.begspn.data(), ny, pos1bk - pos1ob, begbk2);
    const int bgspec[2] = {ctx.rho.bgspec(1), ctx.rho.bgspec(2)};
    int ipos = 0;
    dfdate(bgspec, begbk2, ny, ipos);
    const bool have_span = (ipos + 1) > pos1ob;

    std::vector<double> srs(PLEN, 0.0);

    // The log is taken over [Pos1ob,Posfob] on the X-11 path when the mode is
    // not additive, and on the model-only path when the transform IS the log.
    // `lplog` latches: it records that SOMETHING was logged, and is what the
    // `qslog` key reports.
    auto taklog = [&]() {
        if (lx11) {
            if (muladd != 1) {
                for (int i = pos1ob; i <= posfob; ++i)
                    srs[i - 1] = std::log(srs[i - 1]);
                return true;
            }
        } else if (ctx.arima.lam == 0.0) {
            for (int i = pos1ob; i <= posfob; ++i)
                srs[i - 1] = std::log(srs[i - 1]);
            return true;
        }
        return false;
    };
    auto maybe_log = [&]() { if (llogqs && taklog()) q.lplog = true; };

    auto both_spans = [&](double& full, double& part) {
        qs_diff(srs.data(), pos1ob, posfob, lmodel, nnsedf, nseadf, ny, full);
        if (have_span)
            qs_diff(srs.data(), ipos + 1, posfob, lmodel, nnsedf, nseadf, ny,
                    part);
    };

    // --- the original series (genqs.f:55-83) --------------------------------
    // Lorig is true on the direct pass; it is the indirect (Iagr==4) pass that
    // passes it false, to suppress the original/residual rows.
    {
        const double* series = ctx.inpt.series.data();
        for (int i = 1; i <= PLEN; ++i) srs[i - 1] = series[i - 1];
        maybe_log();
        both_spans(q.qsori, q.qsoris);
    }

    // --- the original adjusted for extreme values and outliers (:88-129) ----
    // This construction IS spcdrv.f:161-176's sp0 input: Stcsi, then either the
    // pseudo-additive REBUILD from the components or the extreme-value fold.
    {
        const double* stcsi = ctx.orisrs.stcsi.data();
        for (int i = 1; i <= PLEN; ++i) srs[i - 1] = stcsi[i - 1];
        if (lx11) {
            if (psuadd) {
                const double* stc = x11_stc_live(ctx);
                const double* sts = ctx.x11srs.sts.data();
                const double* sti = x11_sti_live(ctx);
                for (int i = pos1ob; i <= posfob; ++i)
                    srs[i - 1] = (kfulsm == 2)
                        ? stc[i - 1] * sti[i - 1]
                        : stc[i - 1] * (sts[i - 1] + (sti[i - 1] - 1.0));
            } else {
                addmul(srs.data(), srs.data(), ctx.mq10_stex.data(), pos1bk,
                       posffc, muladd);
            }
        }
        maybe_log();
        both_spans(q.qsori2, q.qsoris2);
    }

    // --- the seasonally adjusted series (:133-168) --------------------------
    bool gosa = false;
    if ((lx11 && kfulsm == 0) || lseats) {
        gosa = true;
        if (lseats) gosa = ctx.seatlg.hvstsa;
    }
    if (gosa) {
        const double* stci = ctx.x11srs.stci.data();
        for (int i = 1; i <= PLEN; ++i) srs[i - 1] = stci[i - 1];
        // Ported asymmetry (genqs.f:148-154): this branch's Lx11 arm is the ONE
        // of the five that does not set `lplog` after logging. Every other
        // series records it. Left as written -- it only matters when the SA
        // series is the sole thing logged, which cannot happen (the original is
        // logged first, under the same condition).
        if (llogqs) taklog();
        both_spans(q.qssadj, q.qssadjs);
    }

    // --- the SA series adjusted for extreme values and outliers (:173-214) --
    if (gosa) {
        const double* stcime = ctx.adxser.stcime.data();
        for (int i = 1; i <= PLEN; ++i) srs[i - 1] = stcime[i - 1];
        // The same unconditional Facls divide spcdrv.f:318 makes before the SA
        // spectrum: the level shift is taken back OUT of the series before its
        // seasonality is tested.
        if (ctx.x11adj.adjls == 1)
            divsub(srs.data(), srs.data(), ctx.x11fac.facls.data(), pos1ob,
                   posfob, muladd);
        maybe_log();
        both_spans(q.qssadj2, q.qssadjs2);
    }

    // --- the irregular, and its EV twin (:218-254) --------------------------
    // These two go STRAIGHT to calcqs -- no differencing, no mean deletion, and
    // no log: an irregular is already stationary around its mode identity, so
    // the oracle only re-centres it (subtract 1 in the ratio modes).
    bool goirr = false;
    if ((lx11 && kfulsm == 0) || lseats) {
        goirr = true;
        if (lseats) goirr = ctx.seatlg.hvstir;
    }
    if (goirr) {
        const double* sti = x11_sti_live(ctx);
        for (int i = pos1ob; i <= posfob; ++i) {
            srs[i - 1] = sti[i - 1];
            if (muladd != 1) srs[i - 1] -= 1.0;
        }
        q.qsirr = calcqs(srs.data(), pos1ob - 1, posfob, ny);
        if (have_span) q.qsirrs = calcqs(srs.data(), ipos, posfob, ny);

        const double* stime = ctx.mq5a_stime.data();
        for (int i = pos1ob; i <= posfob; ++i) {
            srs[i - 1] = stime[i - 1];
            if (muladd != 1) srs[i - 1] -= 1.0;
        }
        q.qsirr2 = calcqs(srs.data(), pos1ob - 1, posfob, ny);
        if (have_span) q.qsirrs2 = calcqs(srs.data(), ipos, posfob, ny);
    }

    return true;
}

// genqs.f:258-265's counterpart at gennpsa.f:111-112.
bool NpStats::lnp() const {
    return !(npsadj == prm::NOTSET && npsadj2 == prm::NOTSET);
}

// CB-26, transcribed: the second test compares two INTEGERs (initialised to
// NOTSET, -32767) against DNOTST, the DOUBLE -999.0 sentinel. Neither can ever
// equal it, so `lnps` is unconditionally TRUE -- which is harmless only because
// every row inside the block re-tests NOTSET individually, so the savelog emits
// nothing extra. The print branch is not so lucky: it writes the "(Series start
// in ...)" header on a run with no span statistics at all.
bool NpStats::lnps() const {
    return !(static_cast<double>(npsadjs) == prm::DNOTST &&
             static_cast<double>(npsadjs2) == prm::DNOTST);
}

namespace {

// npsa.f: optionally log, difference `ndif` times, mean-delete, then threshold
// Kendall's statistic. Returns 1 ("yes, residual seasonality") or 0. NOTE the
// Fortran's `.and.` binds tighter than `.or.`, so the test really is
// (S>24.73 && mq==12) || (S>11.35 && mq==4) -- any other period is always 0.
int npsa(const double* sa, int n1, int nz, bool lmodel, int d, int bd, int mq,
         bool llog) {
    const int ndif = lmodel ? std::max(std::min(2, d + bd), 1) : 1;
    std::vector<double> aux(static_cast<std::size_t>(nz - n1 + 1) + 2, 0.0);
    for (int i = n1; i <= nz; ++i)
        aux[(i - n1 + 1) - 1] = llog ? std::log(sa[i - 1]) : sa[i - 1];
    int k = nz - n1 + 1;
    // Unlike qsdiff this differences ndif times outright -- there is no
    // "difference once, then once more if PosCorr" retry.
    for (int j = 1; j <= ndif; ++j) {
        --k;
        for (int i = 1; i <= k; ++i) aux[i - 1] = aux[(i + 1) - 1] - aux[i - 1];
    }
    double media = 0.0;
    for (int i = 1; i <= k; ++i) media += aux[i - 1];
    media /= k;
    for (int i = 1; i <= k; ++i) aux[i - 1] -= media;
    const double snp = kendalls(aux.data(), k, mq);
    if ((snp > 24.73 && mq == 12) || (snp > 11.35 && mq == 4)) return 1;
    return 0;
}

}  // namespace

// gennpsa.f:1-107.
bool gennpsa(X13Context& ctx, bool lseats) {
    // Same SEATS blocker as genqs (Seatsa / Stocsa).
    if (lseats) return false;

    const int ny = ctx.model.sp;
    const bool lx11 = ctx.captured.has_x11;
    const int muladd = ctx.x11opt.muladd;
    const int kfulsm = ctx.x11opt.kfulsm;
    const bool lmodel = ctx.captured.has_model;
    const bool llogqs = ctx.rho.llogqs;
    const int pos1bk = ctx.x11ptr.pos1bk;
    const int pos1ob = ctx.x11ptr.pos1ob;
    const int posfob = ctx.x11ptr.posfob;
    const int nnsedf = ctx.model.nnsedf;
    const int nseadf = ctx.model.nseadf;

    auto& np = ctx.np;
    np.ran = true;

    // gennpsa.f:52-58 -- lplog is DERIVED here rather than latched as a side
    // effect of a log actually being taken, which is what genqs does. So the
    // two `*log` keys can disagree in principle; on this corpus Llogqs is
    // always off and both read `no`.
    if (llogqs) {
        if (lx11) { if (muladd != 1) np.lplog = true; }
        else if (ctx.arima.lam == 0.0) np.lplog = true;
    }

    int begbk2[2];
    addate(ctx.mdldat.begspn.data(), ny, pos1bk - pos1ob, begbk2);
    const int bgspec[2] = {ctx.rho.bgspec(1), ctx.rho.bgspec(2)};
    int ipos = 0;
    dfdate(bgspec, begbk2, ny, ipos);
    const bool have_span = (ipos + 1) > pos1ob;

    bool gosa = false;
    if ((lx11 && kfulsm == 0) || lseats) {
        gosa = true;
        if (lseats) gosa = ctx.seatlg.hvstsa;
    }
    if (!gosa) return true;

    std::vector<double> srs(PLEN, 0.0);

    // The SA series. Ported asymmetry: this call passes the DERIVED `lplog`
    // and the one below passes the RAW `Llogqs` (gennpsa.f:77 vs :104), so on
    // a non-log X-11 run with logqs=yes the two series are tested on different
    // scales. Left as written.
    {
        const double* stci = ctx.x11srs.stci.data();
        for (int i = 1; i <= PLEN; ++i) srs[i - 1] = stci[i - 1];
        np.npsadj = npsa(srs.data(), pos1ob, posfob, lmodel, nnsedf, nseadf, ny,
                         np.lplog);
        if (have_span)
            np.npsadjs = npsa(srs.data(), ipos + 1, posfob, lmodel, nnsedf,
                              nseadf, ny, np.lplog);
    }

    // ... and its extreme-value twin, with the level shift divided back out.
    {
        const double* stcime = ctx.adxser.stcime.data();
        for (int i = 1; i <= PLEN; ++i) srs[i - 1] = stcime[i - 1];
        if (ctx.x11adj.adjls == 1)
            divsub(srs.data(), srs.data(), ctx.x11fac.facls.data(), pos1ob,
                   posfob, muladd);
        np.npsadj2 = npsa(srs.data(), pos1ob, posfob, lmodel, nnsedf, nseadf,
                          ny, llogqs);
        if (have_span)
            np.npsadjs2 = npsa(srs.data(), ipos + 1, posfob, lmodel, nnsedf,
                               nseadf, ny, llogqs);
    }

    return true;
}

}  // namespace x13
