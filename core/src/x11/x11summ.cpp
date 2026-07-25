// x11summ.cpp -- the X-11 Part-F summary measures + quality statistics (see
// x11summ.hpp). Faithful ports of sumry.f, varian.f, varlog.f, vars.f,
// avedur.f, issame.f, isfals.f, f3cal.f and x11pt4.f's Part-F body.
//
// Every WRITE / table / punch / x11plt in the oracle is deferred (dropped); the
// "diagnostics cannot be generated" writln blocks collapse into the `lsame`
// flag they also set, because that flag is what x11pt4.f:546 returns on.
#include "x11/x11summ.hpp"

#include "common/x13context.hpp"
#include "x11/x11filt.hpp"          // divsub, addmul, logar, antilg, averag, divgud
#include "specparse/specparse.hpp"  // copy, copylg, setlg
#include "numeric/numeric.hpp"      // dpeq, sdev
#include "gen/notset.hpp"           // prm::DNOTST

#include <cmath>
#include <vector>

namespace x13 {

namespace {
constexpr int PLEN = 1020;  // srslen.prm
constexpr int PSP = 12;     // srslen.prm: max seasonal period
}  // namespace

// ---------------------------------------------------------------------------
// sumry.f
// ---------------------------------------------------------------------------
void sumry(const double* x, double* xbar, double* xbar2, double* xsq,
           double* xsd, int iopt, int i, int j, int ny, int muladd,
           const bool* gudval) {
    for (int k = 1; k <= ny; ++k) {
        xbar[k - 1] = 0.0;
        double xcount = 0.0;
        if (iopt <= 1) {
            xbar2[k - 1] = 0.0;
            xsd[k - 1] = 0.0;
        }
        const int kj = j - k;
        for (int l = i; l <= kj; ++l) {
            if (gudval[l - 1]) {
                double c = x[l + k - 1] - x[l - 1];
                if (muladd == 0) c = c * 100.0 / x[l - 1];
                xbar[k - 1] += std::fabs(c);
                if (iopt <= 1) xbar2[k - 1] += c;
                xcount += 1.0;
            }
        }
        if (xcount > 0.0) xbar[k - 1] /= xcount;
        else              xbar[k - 1] = prm::DNOTST;

        if (iopt == 3) continue;
        if (iopt != 1) {
            xsq[k - 1] = dpeq(xbar[k - 1], prm::DNOTST) ? prm::DNOTST
                                                        : xbar[k - 1] * xbar[k - 1];
            if (iopt == 2) continue;  // sumry.f label 10
        }
        if (xcount > 0.0) xbar2[k - 1] /= xcount;
        else              xbar2[k - 1] = prm::DNOTST;
        xsd[k - 1] = 0.0;
        if (muladd == 0) {
            for (int l = i; l <= kj; ++l) {
                if (gudval[l - 1]) {
                    const double d =
                        (x[l + k - 1] - x[l - 1]) / x[l - 1] * 100.0 - xbar2[k - 1];
                    xsd[k - 1] += d * d;
                }
            }
        } else {
            // NOTE the asymmetry: the additive branch has NO Gudval guard (and
            // so divides by an xcount that counted only good pairs). Verbatim.
            for (int l = i; l <= kj; ++l) {
                const double d = x[l + k - 1] - x[l - 1] - xbar2[k - 1];
                xsd[k - 1] += d * d;
            }
        }
        if (xsd[k - 1] > 0.0) xsd[k - 1] = std::sqrt(xsd[k - 1] / xcount);
    }
}

// ---------------------------------------------------------------------------
// varian.f / varlog.f / vars.f -- sums of squares, NOT divided by n.
// ---------------------------------------------------------------------------
double varian(const double* x, int i, int j, int iopt) {
    double ave = 1.0;
    if (iopt != 2) {
        ave = 0.0;
        if (iopt != 1) {
            for (int k = i; k <= j; ++k) ave += x[k - 1];
            ave /= static_cast<double>(j - i + 1);
        }
    }
    double v = 0.0;
    for (int k = i; k <= j; ++k) v += (x[k - 1] - ave) * (x[k - 1] - ave);
    return v;
}

double varlog(const double* x, int i, int j, int iopt, const bool* gudval) {
    double tmp = 0.0, numtmp = 0.0;
    if (iopt != 1) {
        for (int k = i; k <= j; ++k) {
            if (gudval[k - 1] && x[k - 1] > 0.0) {
                tmp += std::log(x[k - 1]);
                numtmp += 1.0;
            }
        }
        if (numtmp > 0.0) tmp /= numtmp;
        else              return prm::DNOTST;
    }
    double v = 0.0;
    for (int k = i; k <= j; ++k) {
        if (gudval[k - 1] && x[k - 1] > 0.0) {
            const double t = std::log(x[k - 1]) - tmp;
            v += t * t;
        }
    }
    return v;
}

double vars(const double* x, int i, int j, int iopt, int muladd,
            const bool* gudval) {
    if (muladd != 1) return varlog(x, i, j, iopt, gudval);
    return varian(x, i, j, iopt);
}

// ---------------------------------------------------------------------------
// avedur.f -- transcribed from the SPAG-mangled GOTO form. The three states are
// the oracle's fall-through entry, label 10 (currently rising) and label 20
// (currently falling); label 30 is the exit.
// ---------------------------------------------------------------------------
void avedur(const double* y, int l, int m, double& adr) {
    enum { S_FLAT, S_RISE, S_FALL, S_DONE } state;
    int i = l;
    double runs = 1.0;
    if (y[i - 1] < y[i]) state = S_RISE;
    else if (dpeq(y[i - 1], y[i])) state = S_FLAT;
    else state = S_FALL;

    while (state != S_DONE) {
        switch (state) {
        case S_FLAT:
            // Leading ties: scan forward without counting a run.
            ++i;
            if (i >= m) { state = S_DONE; break; }
            if (y[i - 1] < y[i]) state = S_RISE;
            else if (y[i - 1] > y[i]) state = S_FALL;
            break;
        case S_RISE:
            ++i;
            if (i >= m) { state = S_DONE; break; }
            if (y[i - 1] > y[i]) { runs += 1.0; state = S_FALL; }
            break;
        case S_FALL:
            ++i;
            if (i >= m) { state = S_DONE; break; }
            if (y[i - 1] < y[i]) { runs += 1.0; state = S_RISE; }
            break;
        default:
            break;
        }
    }
    adr = static_cast<double>(m - l) / runs;
}

// ---------------------------------------------------------------------------
// issame.f / isfals.f
// ---------------------------------------------------------------------------
bool issame(const double* lsrs, int l1, int l2, const bool* gudval) {
    const double base = lsrs[l1 - 1];
    for (int i = l1 + 1; i <= l2; ++i)
        if (gudval[i - 1] && !dpeq(lsrs[i - 1], base)) return false;
    return true;
}

bool isfals(const bool* lsrs, int l1, int l2) {
    for (int i = l1; i <= l2; ++i)
        if (!lsrs[i - 1]) return true;
    return false;
}

// ---------------------------------------------------------------------------
// f3cal.f -- the M1-M11 quality statistics and the composite Q.
// ---------------------------------------------------------------------------
void f3cal(X13Context& ctx, const double* sts, int& ifail) {
    static const double wt[11] = {10.0, 11.0, 10.0, 8.0, 11.0, 10.0,
                                  18.0, 7.0,  7.0,  4.0, 4.0};
    const x11ptr_cmn& p = ctx.x11ptr;
    const x11opt_cmn& opt = ctx.x11opt;
    inpt2_cmn& in2 = ctx.inpt2;
    work2_cmn& w2 = ctx.work2;
    const tests_cmn& t = ctx.tests;
    const int ny = opt.ny;
    double* qu = w2.qu.data();

    ifail = 0;
    int kny = ny / 4;
    qu[0] = (in2.isq(kny) / (1.0 - w2.psq(kny))) / 0.10;
    kny = 12 / ny;
    if (kny < 1) kny = 1;
    qu[1] = (in2.vi / std::fabs(100.0 - in2.vp)) / 0.10;
    qu[2] = (opt.ratic * kny - 1.0) / 2.0;
    double fn = static_cast<double>(p.posfob - p.pos1bk + 1);
    qu[3] = std::fabs(3.0 * (fn - 1.0) / in2.adri - 2.0 * fn + 1.0) /
            (std::sqrt(1.6 * fn - 2.9) * 2.577);

    double rmcd;
    if (opt.mcd == 1) {
        rmcd = 1 + (in2.smic(1) - 1.0) / (in2.smic(1) - in2.smic(2));
        if (rmcd < 0.5) rmcd = 0.5;
        if (rmcd > 1.0) rmcd = 1.0;
    } else {
        const double dsmic = in2.smic(opt.mcd - 1) - in2.smic(opt.mcd);
        if ((dsmic < 0.0 || dpeq(dsmic, 0.0)) && opt.mcd == ny)
            rmcd = kny * 15.5;
        else
            rmcd = opt.mcd + (in2.smic(opt.mcd) - 1.0) / dsmic;
    }
    qu[4] = (rmcd * kny - 0.5) / 5.0;
    if (opt.kfulsm < 2) qu[5] = std::fabs(opt.ratis - 4.0) / 2.5;
    qu[6] = std::sqrt((t.test1 + t.test2) / 2.0);

    w2.nyrs = (p.posfob - p.pos1bk + 1) / ny;
    w2.nn = 7;
    if ((!w2.lstabl) && w2.nyrs >= 6 && opt.kfulsm < 2) {
        w2.nn = 11;
        const int n = 2 - opt.muladd;
        const double sd = sdev(sts, p.pos1bk, p.posfob, 1, n);
        const double ave = 1 - opt.muladd;
        std::vector<double> temp(PLEN, 0.0);
        for (int i = p.pos1bk; i <= p.posfob; ++i)
            temp[i - 1] = (sts[i - 1] - ave) / sd;

        double ct1 = 0.0, ct2 = 0.0, count = 0.0;
        double ave1 = 0.0, ave2 = 0.0, ave3 = 0.0, ave4 = 0.0;
        const int klda = p.pos1bk + ny - 1;
        // `k` is a Fortran local that PERSISTS across outer iterations and is
        // read at :78 even though the inner DO that sets it can execute zero
        // times. It cannot here (Nyrs>=6 guarantees at least one pair), but the
        // scope is kept faithful rather than moved inside the loop.
        int k = 0;
        for (int i = p.pos1bk; i <= klda; ++i) {
            double ct = 0.0;
            int i1 = i + ny;
            for (int j = i1; j <= p.posfob; j += ny) {
                ct += 1.0;
                count += 1.0;
                const double diff = std::fabs(temp[j - 1] - temp[j - ny - 1]);
                k = j;
                ave1 += diff;
            }
            ave2 += std::fabs(temp[k - 1] - temp[i - 1]) / ct;
            k = k - 2 * ny;
            int j = k - 3 * ny;
            if (j >= p.pos1bk) {
                ave3 += std::fabs(temp[k - 1] - temp[j - 1]) / 3.0;
                ct1 += 1.0;
                j = j + ny;
                for (int l = j; l <= k; l += ny) {
                    i1 = l - ny;
                    if (i1 >= p.pos1bk) {
                        ct2 += 1.0;
                        ave4 += std::fabs(temp[l - 1] - temp[i1 - 1]);
                    }
                }
            }
        }
        ave1 /= count;
        ave2 /= static_cast<double>(ny);
        if (!dpeq(ct1, 0.0)) {
            ave3 /= ct1;
            ave4 /= ct2;
        }
        qu[7] = 10.0 * ave1;
        qu[8] = 10.0 * ave2;
        qu[9] = 10.0 * ave4;
        qu[10] = 10.0 * ave3;
    }

    w2.qual = 0.0;
    double twt = 0.0;
    for (int i = 1; i <= 11; ++i) {
        if (i <= w2.nn) {
            if (qu[i - 1] < 0.0) qu[i - 1] = 0.0;
            if (qu[i - 1] > 3.0) qu[i - 1] = 3.0;
            if (qu[i - 1] >= 1.0) ifail += 1;
            // f3cal.f's GO TO 10 skips the twt accumulation as well as the Qual
            // one, so excluding M6 also drops its weight from the divisor.
            if (((!w2.l3x5) || opt.kfulsm == 2) && i == 6) continue;
            w2.qual += qu[i - 1] * wt[i - 1];
        } else if (i < 10) {
            w2.qual += qu[6] * wt[i - 1];
        } else if (i == 10) {
            w2.qual += qu[0] * wt[i - 1];
        } else {
            w2.qual += qu[1] * wt[i - 1];
        }
        twt += wt[i - 1];
    }
    w2.qual /= twt;
    w2.kfail = ifail;

    double twt2 = 11.0;
    if (w2.lstabl || w2.nyrs < 6 || opt.kfulsm == 2) twt2 = 15.0;
    w2.q2m2 = ((w2.qual * twt) - (qu[1] * twt2)) / (twt - twt2);
}

// ---------------------------------------------------------------------------
// x11pt4.f, PART E (:162-:319)
// ---------------------------------------------------------------------------
void x11pt4_etables(X13Context& ctx, const double* stc_int,
                    const double* stc2_int, bool lttc) {
    const x11ptr_cmn& p = ctx.x11ptr;
    const x11opt_cmn& opt = ctx.x11opt;
    const x11adj_cmn& adj = ctx.x11adj;
    const priusr_cmn& pu = ctx.priusr;
    const force_cmn& frc = ctx.force;

    const int pos1ob = p.pos1ob, posfob = p.posfob;
    const int pos1bk = p.pos1bk, posffc = p.posffc;
    const int muladd = opt.muladd;
    const double* series = ctx.inpt.series.data();
    const double* stci = ctx.x11srs.stci.data();
    const double* stome = ctx.adxser.stome.data();
    const double* stcime = ctx.adxser.stcime.data();
    bool* gudval = ctx.goodob.gudval.data();

    // x11pt4.f:169-176 -- with percent changes to compute and a pseudo-additive
    // adjustment or a user constant in play, re-check for zeroes first (a zero
    // denominator would make every change DNOTST from that point on).
    if (muladd != 1 && (ctx.x11msc.psuadd || !dpeq(ctx.adj.cnstnt, prm::DNOTST)))
        chkzro(series, stci, ctx.adxser.stci2.data(), ctx.adxser.stcirn.data(),
               ctx.orisrs.stocal.data(), pos1bk, posffc, opt.kfulsm, frc.iyrt,
               frc.lrndsa, gudval);

    const int mfda = pos1ob + 1;
    ctx.x11_e5.assign(PLEN, 0.0);
    ctx.x11_e6.assign(PLEN, 0.0);
    ctx.x11_e7.assign(PLEN, 0.0);
    ctx.x11_e8.assign(PLEN, 0.0);
    ctx.x11_e6a.clear();
    ctx.x11_e6r.clear();

    // E5: changes in the original series.
    change(series, ctx.x11_e5.data(), mfda, posfob, muladd, gudval);
    if (opt.kfulsm == 0) {
        // E6: changes in the seasonally adjusted series.
        change(stci, ctx.x11_e6.data(), mfda, posfob, muladd, gudval);
        // E6.A / E6.R: the same for the forced and the rounded SA series.
        if (frc.iyrt > 0) {
            ctx.x11_e6a.assign(PLEN, 0.0);
            change(ctx.adxser.stci2.data(), ctx.x11_e6a.data(), mfda, posfob,
                   muladd, gudval);
        }
        if (frc.lrndsa) {
            ctx.x11_e6r.assign(PLEN, 0.0);
            change(ctx.adxser.stcirn.data(), ctx.x11_e6r.data(), mfda, posfob,
                   muladd, gudval);
        }
    }
    // E7: changes in the final trend-cycle. If a level shift (or, with Lttc, a
    // temporary change) was removed pre-adjustment it belongs in the trend, so
    // the changes are taken on the FOLDED trend Stc2 instead of Stc.
    // NOTE the Fortran precedence at x11pt4.f:241-243: `.and.` binds tighter
    // than `.or.`, so `Iagr.lt.4` qualifies ONLY the Nustad/Lprntr clause, not
    // the Finls/Adjls one. Reproduced verbatim.
    const bool e7_from_stc2 =
        (((!adj.finls) && adj.adjls == 1) ||
         (pu.nustad > 0 && ctx.prior.lprntr && ctx.agr.iagr < 4)) ||
        (ctx.agr.iagr == 4 && ctx.agr.lindls) ||
        (lttc && adj.adjtc == 1 && !adj.fintc);
    change(e7_from_stc2 ? stc2_int : stc_int, ctx.x11_e7.data(), mfda, posfob,
           muladd, gudval);
    // E8: changes in the calendar-adjusted original series.
    change(ctx.orisrs.stocal.data(), ctx.x11_e8.data(), mfda, posfob, muladd,
           gudval);

    // x11pt4.f:265-268 -- with a user constant, every observation counts as good
    // from here on (the constant shifts the series away from zero). gudbak is
    // restored by x11pt4_partf's own copylg, so the save is kept on ctx.
    if (muladd != 1 && !dpeq(ctx.adj.cnstnt, prm::DNOTST)) {
        ctx.x11_gudbak.assign(gudval, gudval + PLEN);
        setlg(true, PLEN, gudval);
    } else {
        ctx.x11_gudbak.clear();
    }

    // E11: a more robust seasonally adjusted series (x11pt4.f:272-274). Note the
    // arithmetic is a plain subtract/add in EVERY mode, multiplicative included.
    ctx.x11_e11.assign(PLEN, 0.0);
    for (int i = pos1ob; i <= posfob; ++i)
        ctx.x11_e11[i - 1] = series[i - 1] - stome[i - 1] + stcime[i - 1];

    // E18: the final adjustment ratios A1 / D11 (x11pt4.f:281-301). A zero SA
    // value with a nonzero original has no ratio -- DNOTST -- and sets pre18b,
    // which is what makes the oracle also emit the total-factor table below.
    ctx.x11_e18.assign(PLEN, 0.0);
    bool pre18b = false;
    const bool cmp = (ctx.agr.iagr >= 4);
    const double* obs = cmp ? ctx.agrsrs.o.data() : series;
    for (int i = pos1ob; i <= posffc; ++i) {
        const double thisob = obs[i - 1];
        if (dpeq(stci[i - 1], 0.0)) {
            if (dpeq(thisob, 0.0)) {
                ctx.x11_e18[i - 1] = 1.0;
            } else {
                ctx.x11_e18[i - 1] = prm::DNOTST;
                pre18b = true;
            }
        } else {
            if (dpeq(thisob, 0.0) || thisob < 0.0) pre18b = true;
            ctx.x11_e18[i - 1] = thisob / stci[i - 1];
        }
    }
    // EB: the total adjustment factors (x11pt4.f:309-319). Same quantity as E18
    // but formed through divsub, so it is a DIFFERENCE in additive mode where
    // E18 is always a ratio. The oracle builds it whenever pre18b fired or the
    // table was requested; building it unconditionally is harmless here (nothing
    // downstream reads it) and keeps the harness able to punch it.
    (void)pre18b;
    ctx.x11_eb.assign(PLEN, 0.0);
    divsub(ctx.x11_eb.data(), obs, stci, pos1ob, posffc, muladd);

    ctx.x11_etables_set = true;
}

// ---------------------------------------------------------------------------
// x11pt4.f, PART F (:320-:713)
// ---------------------------------------------------------------------------
bool x11pt4_partf(X13Context& ctx, const double* sti_int, const double* stc_int) {
    const x11ptr_cmn& p = ctx.x11ptr;
    x11opt_cmn& opt = ctx.x11opt;
    const x11adj_cmn& adj = ctx.x11adj;
    const x11log_cmn& xlog = ctx.x11log;
    const hiddn_cmn& hid = ctx.hiddn;
    inpt2_cmn& in2 = ctx.inpt2;
    work2_cmn& w2 = ctx.work2;

    const int pos1bk = p.pos1bk, pos1ob = p.pos1ob;
    const int posfob = p.posfob, posffc = p.posffc;
    const int ny = opt.ny, muladd = opt.muladd;

    // Working copies -- see the header note on why these are not the live
    // buffers. Sti/Stc come from the caller (the internal, pre-publication D13
    // and D12); the rest are copied straight off the COMMONs.
    std::vector<double> vseries(ctx.inpt.series.data(),
                                ctx.inpt.series.data() + PLEN);
    std::vector<double> vstci(ctx.x11srs.stci.data(),
                              ctx.x11srs.stci.data() + PLEN);
    std::vector<double> vstc(stc_int, stc_int + PLEN);
    std::vector<double> vsti(sti_int, sti_int + PLEN);
    std::vector<double> vstome(ctx.adxser.stome.data(),
                               ctx.adxser.stome.data() + PLEN);
    std::vector<double> vstcime(ctx.adxser.stcime.data(),
                                ctx.adxser.stcime.data() + PLEN);
    double* series = vseries.data();
    double* stci = vstci.data();
    double* stc = vstc.data();
    double* sti = vsti.data();
    double* stome = vstome.data();
    double* stcime = vstcime.data();
    const double* stime = ctx.mq5a_stime.data();
    const double* sts = ctx.x11srs.sts.data();
    const double* faccal = ctx.x11fac.faccal.data();
    const double* facls = ctx.x11fac.facls.data();
    const double* facao = ctx.x11fac.facao.data();
    const double* factc = ctx.x11fac.factc.data();
    const double* facusr = ctx.x11fac.facusr.data();
    bool* gudval = ctx.goodob.gudval.data();

    std::vector<double> vtemp(PLEN, 0.0), vtrend(PLEN, 0.0);
    double* temp = vtemp.data();
    double* trend = vtrend.data();
    double dvec[PSP] = {0.0};
    double ombar2[PSP] = {0.0}, ombrsq[PSP] = {0.0}, ombrsd[PSP] = {0.0};
    double imbar2[PSP] = {0.0}, imbrsd[PSP] = {0.0};

    bool lsame = false;
    opt.kpart = 6;

    // x11pt4.f:332-336. CB-18: `allgud` is set from isfals(Gudval,...), which is
    // TRUE when at least one observation is BAD -- the opposite of what the name
    // and the branches that consume it want (allgud picks the unguarded divsub,
    // !allgud the good-obs-only divgud). Reproduced verbatim; unreachable in
    // this port because the Cnstnt path is walled in x11pt3.
    bool allgud = true;
    if (muladd != 1 && !dpeq(ctx.adj.cnstnt, prm::DNOTST)) {
        // x11pt4.f:334 -- restore the flags Part E stashed before forcing them
        // all true (ctx.x11_gudbak; gudbak is a local in the oracle).
        if (ctx.x11_gudbak.size() == static_cast<std::size_t>(PLEN))
            for (int i = 1; i <= PLEN; ++i)
                gudval[i - 1] = ctx.x11_gudbak[static_cast<std::size_t>(i - 1)];
        allgud = isfals(gudval, pos1ob, posfob);
    }

    // --- Prior-adjustment factors (x11pt4.f:338-348) ---
    if (ctx.prior.kfmt != 0) {
        sumry(ctx.inpt.sprior.data(), w2.pbar.data(), dvec, w2.psq.data(), dvec,
              2, pos1ob, posfob, ny, muladd, gudval);
        in2.vp = vars(ctx.inpt.sprior.data(), pos1ob, posfob, 0, muladd, gudval);
    } else {
        for (int i = 1; i <= ny; ++i) {
            w2.pbar(i) = 0.0;
            w2.psq(i) = 0.0;
        }
        in2.vp = 0.0;
    }

    // --- Calendar factors (x11pt4.f:349-360) ---
    const bool no_cal =
        ((!xlog.axrgtd && opt.kswv == 0 && adj.adjtd <= 0) &&
         !(adj.adjhol == 1 || opt.khol == 2 || (hid.ixreg > 0 && xlog.axrghl))) ||
        (ctx.agr.iagr >= 4 && !ctx.agr.lindcl);
    if (no_cal) {
        in2.vtd = 0.0;
        for (int i = 1; i <= ny; ++i) {
            in2.tdbar(i) = 0.0;
            in2.tdsq(i) = 0.0;
        }
    } else {
        sumry(faccal, in2.tdbar.data(), dvec, in2.tdsq.data(), dvec, 2, pos1ob,
              posfob, ny, muladd, gudval);
        in2.vtd = vars(faccal, pos1ob, posfob, 1, muladd, gudval);
    }

    // --- Irregular (x11pt4.f:361-362). Isq is overwritten from Stime below. ---
    sumry(sti, in2.ibar.data(), in2.ibar2.data(), in2.isq.data(),
          in2.isd.data(), 0, pos1ob, posfob, ny, muladd, gudval);
    avedur(sti, pos1ob, posfob, in2.adri);

    // --- Modified original E1 (x11pt4.f:373-392) ---
    if (allgud) {
        if (adj.adjls == 1) divsub(stome, stome, facls, pos1bk, posffc, muladd);
        if (adj.adjusr == 1) divsub(stome, stome, facusr, pos1bk, posffc, muladd);
    } else {
        copy(stome, posffc, 1, temp);
        if (adj.adjls == 1) divgud(stome, stome, facls, pos1bk, posffc, gudval);
        if (adj.adjusr == 1) divgud(stome, stome, facusr, pos1bk, posffc, gudval);
    }
    sumry(stome, in2.ombar.data(), ombar2, ombrsq, ombrsd, 0, pos1ob, posfob, ny,
          muladd, gudval);
    if (issame(stome, pos1ob, posfob, gudval)) lsame = true;

    // --- Modified irregular E3 (x11pt4.f:393-407) ---
    sumry(stime, in2.imbar.data(), imbar2, in2.isq.data(), imbrsd, 0, pos1ob,
          posfob, ny, muladd, gudval);
    in2.vi = vars(stime, pos1ob, posfob, 1, muladd, gudval);
    if (dpeq(in2.vi, 0.0) || issame(sti, pos1ob, posfob, gudval) ||
        dpeq(in2.vi, prm::DNOTST))
        lsame = true;

    if (allgud) {
        if (adj.adjls == 1) addmul(stome, stome, facls, pos1bk, posffc, muladd);
        if (adj.adjusr == 1) addmul(stome, stome, facusr, pos1bk, posffc, muladd);
    } else {
        copy(temp, posffc, 1, stome);
    }

    // --- Seasonal + trend (x11pt4.f:416-440) ---
    if (opt.kfulsm == 2) {
        for (int i = 1; i <= ny; ++i) {
            in2.sbar(i) = 0.0;
            in2.sbar2(i) = 0.0;
            in2.ssd(i) = 0.0;
            in2.ssq(i) = 0.0;
        }
        in2.vs = 0.0;
    } else {
        sumry(sts, in2.sbar.data(), in2.sbar2.data(), in2.ssq.data(),
              in2.ssd.data(), 0, pos1ob, posfob, ny, muladd, gudval);
        in2.vs = vars(sts, pos1ob, posfob, 1, muladd, gudval);
    }
    sumry(stc, in2.cbar.data(), in2.cbar2.data(), in2.csq.data(),
          in2.csd.data(), 0, pos1ob, posfob, ny, muladd, gudval);
    if (issame(stc, pos1ob, posfob, gudval)) lsame = true;

    // --- Remove the linear trend (linear percentage growth when
    //     multiplicative) from the trend-cycle (x11pt4.f:445-477). NOTE the
    //     log/antilog ROUND TRIP on Stc: it is not the identity, and the oracle
    //     keeps the perturbed values. ---
    if (muladd != 1) {
        if (!dpeq(ctx.adj.cnstnt, prm::DNOTST))
            for (int i = pos1ob; i <= posfob; ++i) stc[i - 1] += ctx.adj.cnstnt;
        logar(stc, pos1ob, posfob);
    }
    double tmp1 = static_cast<double>(-pos1ob - posfob);
    double tmp2 = 0.0;
    for (int i = pos1ob; i <= posfob; ++i)
        tmp2 += stc[i - 1] * (2.0 * static_cast<double>(i) + tmp1);
    const double fn = static_cast<double>(posfob - pos1ob + 1);
    tmp1 = 6.0 * tmp2 / (fn * (fn * fn - 1.0));
    for (int i = pos1ob; i <= posffc; ++i)
        trend[i - 1] = tmp1 * (static_cast<double>(i - pos1bk) + 1.0);
    if (muladd != 1) {
        antilg(stc, pos1ob, posfob);
        antilg(trend, pos1ob, posfob);
    }
    divsub(temp, stc, trend, pos1ob, posfob, muladd);
    if (muladd != 1 && !dpeq(ctx.adj.cnstnt, prm::DNOTST)) {
        for (int i = pos1ob; i <= posfob; ++i) {
            stc[i - 1] -= ctx.adj.cnstnt;
            trend[i - 1] -= ctx.adj.cnstnt;
        }
    }
    avedur(stc, pos1ob, posfob, in2.adrc);
    in2.vc = vars(temp, pos1ob, posfob, 0, muladd, gudval);

    // --- Original series, outlier/user effects removed (x11pt4.f:479-526) ---
    if (allgud) {
        if (adj.adjls == 1) divsub(series, series, facls, pos1bk, posffc, muladd);
        if (adj.adjao == 1) divsub(series, series, facao, pos1bk, posffc, muladd);
        if (adj.adjtc == 1) divsub(series, series, factc, pos1bk, posffc, muladd);
        if (adj.adjusr == 1) divsub(series, series, facusr, pos1bk, posffc, muladd);
    } else {
        copy(series, posffc, 1, temp);
        if (adj.adjls == 1) divgud(series, series, facls, pos1bk, posffc, gudval);
        if (adj.adjao == 1) divgud(series, series, facao, pos1bk, posffc, gudval);
        if (adj.adjtc == 1) divgud(series, series, factc, pos1bk, posffc, gudval);
        if (adj.adjusr == 1) divgud(series, series, facusr, pos1bk, posffc, gudval);
    }
    if (issame(series, pos1ob, posfob, gudval)) lsame = true;
    sumry(series, in2.obar.data(), in2.obar2.data(), in2.osq.data(),
          in2.osd.data(), 0, pos1ob, posfob, ny, muladd, gudval);
    if (allgud) {
        if (adj.adjls == 1) addmul(series, series, facls, pos1bk, posffc, muladd);
        if (adj.adjao == 1) addmul(series, series, facao, pos1bk, posffc, muladd);
        if (adj.adjtc == 1) addmul(series, series, factc, pos1bk, posffc, muladd);
        if (adj.adjusr == 1) addmul(series, series, facusr, pos1bk, posffc, muladd);
    } else {
        copy(temp, posffc, 1, series);
    }

    // --- The reference variance: E1 with the linear trend removed. Everything
    //     from here is expressed as a percentage of it (x11pt4.f:528-552). ---
    divsub(temp, stome, trend, pos1ob, posfob, muladd);
    const double vo = vars(temp, pos1ob, posfob, 0, muladd, gudval) / 100.0;
    if (dpeq(vo, 0.0) || dpeq(vo, prm::DNOTST)) lsame = true;
    if (lsame) return false;  // x11pt4.f:546

    in2.vp /= vo;
    in2.vtd /= vo;
    in2.vc /= vo;
    in2.vs /= vo;
    in2.vi /= vo;
    in2.rv = in2.vp + in2.vtd + in2.vc + in2.vs + in2.vi;

    for (int i = 1; i <= ny; ++i) {
        if (dpeq(in2.isq(i), prm::DNOTST) || dpeq(in2.csq(i), prm::DNOTST) ||
            dpeq(in2.ssq(i), prm::DNOTST) || dpeq(w2.psq(i), prm::DNOTST) ||
            dpeq(in2.tdsq(i), prm::DNOTST)) {
            if (!dpeq(in2.isq(i), prm::DNOTST)) in2.isq(i) = prm::DNOTST;
            if (!dpeq(in2.csq(i), prm::DNOTST)) in2.csq(i) = prm::DNOTST;
            if (!dpeq(in2.ssq(i), prm::DNOTST)) in2.ssq(i) = prm::DNOTST;
            if (!dpeq(w2.psq(i), prm::DNOTST)) w2.psq(i) = prm::DNOTST;
            if (!dpeq(in2.tdsq(i), prm::DNOTST)) in2.tdsq(i) = prm::DNOTST;
            in2.osq2(i) = prm::DNOTST;
        } else {
            in2.osq2(i) = in2.isq(i) + in2.csq(i) + in2.ssq(i) + w2.psq(i) +
                          in2.tdsq(i);
            in2.isq(i) /= in2.osq2(i);
            in2.csq(i) /= in2.osq2(i);
            in2.ssq(i) /= in2.osq2(i);
            w2.psq(i) /= in2.osq2(i);
            in2.tdsq(i) /= in2.osq2(i);
            // Relative to the MODIFIED original's squared changes, not Osq --
            // the Osq line above it is commented out in the oracle.
            in2.osq2(i) /= ombrsq[i - 1];
        }
        // I/C ratios.
        if (dpeq(in2.cbar(i), prm::DNOTST) || dpeq(in2.ibar(i), prm::DNOTST))
            in2.smic(i) = prm::DNOTST;
        else
            in2.smic(i) = in2.ibar(i) / in2.cbar(i);
    }

    // --- Seasonally adjusted series (x11pt4.f:583-618) ---
    if (allgud) {
        if (!adj.finls && adj.adjls == 1)
            divsub(stci, stci, facls, pos1bk, posffc, muladd);
        if (!adj.finao && adj.adjao == 1)
            divsub(stci, stci, facao, pos1bk, posffc, muladd);
        if (!adj.fintc && adj.adjtc == 1)
            divsub(stci, stci, factc, pos1bk, posffc, muladd);
        if (!adj.finusr && adj.adjusr == 1)
            divsub(stci, stci, facusr, pos1bk, posffc, muladd);
    } else {
        copy(stci, posffc, 1, temp);
        if (!adj.finls && adj.adjls == 1)
            divgud(stci, stci, facls, pos1bk, posffc, gudval);
        if (!adj.finao && adj.adjao == 1)
            divgud(stci, stci, facao, pos1bk, posffc, gudval);
        if (!adj.fintc && adj.adjtc == 1)
            divgud(stci, stci, factc, pos1bk, posffc, gudval);
        if (!adj.finusr && adj.adjusr == 1)
            divgud(stci, stci, facusr, pos1bk, posffc, gudval);
    }
    sumry(stci, in2.cibar.data(), in2.cibar2.data(), dvec, in2.cisd.data(), 1,
          pos1ob, posfob, ny, muladd, gudval);
    avedur(stci, pos1ob, posfob, in2.adrci);
    if (allgud) {
        if (!adj.finls && adj.adjls == 1)
            addmul(stci, stci, facls, pos1bk, posffc, muladd);
        if (!adj.finao && adj.adjao == 1)
            addmul(stci, stci, facao, pos1bk, posffc, muladd);
        if (!adj.fintc && adj.adjtc == 1)
            addmul(stci, stci, factc, pos1bk, posffc, muladd);
        if (!adj.finusr && adj.adjusr == 1)
            addmul(stci, stci, facusr, pos1bk, posffc, muladd);
    } else {
        copy(temp, posffc, 1, stci);
    }

    // --- Modified SA series E2 (x11pt4.f:620-654). The AO/TC divides here are
    //     commented out in the oracle; only LS and user effects are removed. ---
    if (allgud) {
        if (!adj.finls && adj.adjls == 1)
            divsub(stcime, stcime, facls, pos1bk, posffc, muladd);
        if (!adj.finusr && adj.adjusr == 1)
            divsub(stcime, stcime, facusr, pos1bk, posffc, muladd);
    } else {
        copy(stcime, posffc, 1, temp);
        if (!adj.finls && adj.adjls == 1)
            divgud(stcime, stcime, facls, pos1bk, posffc, gudval);
        if (!adj.finusr && adj.adjusr == 1)
            divgud(stcime, stcime, facusr, pos1bk, posffc, gudval);
    }
    sumry(stcime, in2.cimbar.data(), dvec, dvec, dvec, 3, pos1ob, posfob, ny,
          muladd, gudval);
    if (allgud) {
        if (!adj.finls && adj.adjls == 1)
            addmul(stcime, stcime, facls, pos1bk, posffc, muladd);
        if (!adj.finusr && adj.adjusr == 1)
            addmul(stcime, stcime, facusr, pos1bk, posffc, muladd);
    } else {
        // CB-19: this ELSE is meant to RESTORE Stcime from the saved copy, as
        // the three analogous blocks above it do (`copy(Temp,...,Stci)`), but
        // the arguments are the wrong way round -- it copies Stcime INTO Temp,
        // so the LS/user divide is never undone and Temp is clobbered as well.
        // Verbatim (x11pt4.f:653); unreachable here, allgud is always true.
        copy(stcime, posffc, 1, temp);
    }

    // --- MCD: the shortest span over which the I/C ratio drops below 1 ---
    opt.mcd = ny;
    bool hit_floor = false;
    while (in2.smic(opt.mcd) < 1.0) {
        if (opt.mcd == 1) { hit_floor = true; break; }
        opt.mcd -= 1;
    }
    if (!hit_floor) {
        opt.mcd += 1;
        if (opt.mcd > ny) opt.mcd = ny;
    }
    int n = opt.mcd;
    if (n > 6) n = 6;
    const int m = 2 - n + n / 2 * 2;

    std::vector<double> vstmcd(PLEN, 0.0);
    double* stmcd = vstmcd.data();
    averag(stci, stmcd, pos1bk, posffc, m, n);
    const int mfda = pos1ob + n / 2;
    const int mlda = posfob - n / 2;
    int mldaf = posffc - n / 2;
    if (mldaf > posfob) mldaf = posfob;
    sumry(stmcd, in2.smbar.data(), in2.smbar2.data(), dvec, in2.smsd.data(), 1,
          mfda, mlda, ny, muladd, gudval);
    avedur(stmcd, mfda, mlda, in2.adrmcd);
    opt.kpart = 6;
    // x11pt4.f:694-696 -- Stmcd is overwritten from the /work/ scratch AFTER the
    // F1 table has been printed, so this leaves the MCD average holding whatever
    // Temp last held (E1 detrended, from :528). Dead here; kept for the record.
    for (int i = mfda; i <= mldaf; ++i) stmcd[i - 1] = temp[i - 1];

    // --- Autocorrelations of the irregular, lags 1..Ny+2 (x11pt4.f:701-712) ---
    w2.nn = 2 - muladd;  // reused as a scratch; f3cal resets it to 7 or 11
    const double ebar = static_cast<double>(1 - muladd);
    const double vtmp = varian(sti, pos1ob, posfob, w2.nn) / fn;
    n = ny + 2;
    for (int i = 1; i <= n; ++i) w2.autoc(i) = 0.0;
    for (int i = 1; i <= n; ++i) {
        const int ij = pos1ob + i;
        for (int j = ij; j <= posfob; ++j)
            w2.autoc(i) += (sti[j - 1] - ebar) * (sti[j - i - 1] - ebar);
        w2.autoc(i) /= ((fn - i) * vtmp);
    }

    int ifail = 0;
    f3cal(ctx, sts, ifail);
    return true;
}

}  // namespace x13
