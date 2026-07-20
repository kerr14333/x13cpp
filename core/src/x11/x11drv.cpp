// x11drv.cpp -- X-11 Tier 4 driver routines (see x11drv.hpp). Faithful port of
// the vendored oracle Fortran: vtc.f.
//
// Index convention: 0-based C pointers, Fortran index i -> element [i-1]; range
// args (Pos1bk/Posffc/ib/ie) stay Fortran 1-based. Loop and float op order are
// preserved verbatim (the I/C ratio feeds the trend-length selection; no
// algebraic simplification). COMMON state lives on ctx.x11opt/x11ptr/x11msc.
#include "x11/x11drv.hpp"

#include "common/x13context.hpp"
#include "x11/x11filt.hpp"       // hndtrn, divsub
#include "x11/x11seas.hpp"       // vsfa, vsfb
#include "x11/x11xtrm.hpp"       // xtrm, replac, vtest, entsch
#include "transform/transform.hpp"  // invfcn (inverse Box-Cox)
#include "specparse/specparse.hpp"  // copy, setdp
#include "numeric/numeric.hpp"   // dpeq, dpow_ri
#include "gen/model.hpp"         // prm:: regression-type constants (PRGT*)

#include <algorithm>
#include <cmath>

namespace x13 {

namespace {
// srslen.prm: PLEN = POBS + 2*PFCST -- the back/forecast-padded buffer length.
// vtc's /work/ Temp scratch is dimensioned PLEN in the Fortran.
constexpr int PLEN = 1020;
}  // namespace

// extend.f -- append the model forecasts/backcasts to the observed series in the
// padded X-11 buffer (Orix). For multiplicative/log-additive SA the forecast and
// backcast values must be positive; a non-positive value cancels the extension
// (Extok=false) -- the Fortran warning WRITEs are deferred (dropped), the Extok
// side effect is preserved. The observed series is copied at Pos1ob regardless.
void extend(X13Context& ctx, double* trnsrs, int* begxy, double* orix,
            bool& extok, double lam, const double* fcst, const double* bcst) {
    extend_cmn& ext = ctx.extend;
    x11ptr_cmn& ptr = ctx.x11ptr;
    const int muladd = ctx.x11opt.muladd;
    const bool psuadd = ctx.x11msc.psuadd;
    const int nspobs = ctx.mdldat.nspobs;
    constexpr int PFCST = 120;  // srslen.prm: 10*PSP

    extok = true;

    // Multiplicative/log SA + non-log transform: a non-positive forecast blocks
    // the forecast extension (deferred warning dropped).
    if (ext.nfcst > 0) {
        if ((!dpeq(lam, 0.0)) && muladd != 1) {
            int i = 1;
            while (i <= ext.nfcst && extok) {
                if (psuadd && fcst[i - 1] < 0.0) {
                    extok = false;
                } else if (fcst[i - 1] <= 0.0) {
                    extok = false;
                }
                i = i + 1;
            }
        }
    }
    // Same guard for the backcasts.
    if (ext.nbcst > 0 && extok) {
        if ((!dpeq(lam, 0.0)) && muladd != 1) {
            int i = 1;
            while (i <= ext.nbcst && extok) {
                if (psuadd && bcst[i - 1] < 0.0) {
                    extok = false;
                } else if (bcst[i - 1] <= 0.0) {
                    extok = false;
                }
                i = i + 1;
            }
        }
    }

    // Copy the transformed series into the original vector at Pos1ob.
    copy(trnsrs, nspobs, 1, orix + (ptr.pos1ob - 1));

    if (!extok) return;
    // Append forecasts after Posfob.
    if (ext.nfcst > 0) copy(fcst, ext.nfcst, 1, orix + ptr.posfob);

    // Append (reversed) backcasts at Pos1bk, adjusting the Xy start date.
    if (ext.nbcst > 0) {
        begxy[0] = ext.begbak(1);  // YR
        begxy[1] = ext.begbak(2);  // MO
        double bcst2[PFCST];
        revrse(bcst, ext.nbcst, 1, bcst2);
        copy(bcst2, ext.nbcst, 1, orix + (ptr.pos1bk - 1));
    }
}

// x11int.f -- initialize the X-11 factor/series/weight arrays before a run:
// unit-value (or 0 additive) for the multiplicative factors, 0 for trend/weight
// buffers, and copy any prior adjustment into Sprior.
void x11int(X13Context& ctx) {
    x11opt_cmn& opt = ctx.x11opt;
    x11srs_cmn& srs = ctx.x11srs;
    x11fac_cmn& fac = ctx.x11fac;
    xtrm_cmn& xt = ctx.xtrm;
    adj_cmn& adj = ctx.adj;
    // srslen.prm: PY1 = PYRS+1 = 86 (PYRS = PYR1+20 = 85). Matches Stdev's size.
    constexpr int PY1 = 86;

    double rinit = 1.0;
    if (opt.muladd == 1) rinit = 0.0;

    // Multiplicative factor / seasonal-input arrays -> rinit (the identity).
    setdp(rinit, PLEN, srs.sts.data());
    setdp(rinit, PLEN, srs.stsi.data());
    setdp(rinit, PLEN, srs.sti.data());
    setdp(rinit, PLEN, fac.stptd.data());
    // (Fortran also zeroes /work/ Temp here; that is per-routine scratch in this
    // port -- no shared COMMON -- so its init is the callee's concern, skipped.)
    setdp(rinit, PLEN, fac.factd.data());
    setdp(rinit, PLEN, fac.facao.data());
    setdp(rinit, PLEN, fac.facls.data());
    setdp(rinit, PLEN, fac.factc.data());
    setdp(rinit, PLEN, fac.facso.data());
    setdp(rinit, PLEN, fac.facsea.data());
    setdp(rinit, PLEN, fac.facusr.data());
    setdp(rinit, PLEN, fac.fachol.data());
    setdp(rinit, PLEN, fac.facxhl.data());
    setdp(rinit, PLEN, fac.x11hol.data());
    setdp(rinit, PLEN, fac.faccal.data());

    // Trend / weight buffers -> 0 (and Stdev over PY1). /mq10/ Stex is likewise
    // per-routine scratch here, skipped.
    setdp(0.0, PLEN, srs.stc.data());
    setdp(0.0, PLEN, xt.stwt.data());
    setdp(0.0, PLEN, srs.stci.data());
    setdp(0.0, PY1, xt.stdev.data());

    // Copy adjustment factors into Sprior (reverse copy, inc=-1).
    if (adj.nadj > 0)
        copy(adj.adj.data(), PLEN - adj.setpri + 1, -1,
             ctx.inpt.sprior.data() + (adj.setpri - 1));
}

// setxpt.f -- set the X-11 span "pointers" (Pos1bk/Pos1ob/Posfob/Posffc) that
// bracket backcasts / data / forecasts inside the padded buffer.
void setxpt(X13Context& ctx, int nf2, bool lsadj, int fctdrp) {
    extend_cmn& ext = ctx.extend;
    x11ptr_cmn& ptr = ctx.x11ptr;
    const int lsp = ctx.lzero.lsp;

    int nspobs = ext.nofpob - nf2;
    ptr.pos1bk = ext.nbcst2 - ext.nbcst + lsp;
    ptr.pos1ob = ext.nbcst2 + lsp;
    ptr.posfob = ext.nbcst2 + nspobs + lsp - 1;
    ptr.posffc = ext.nbcst2 + nspobs + ext.nfcst + lsp - 1;
    if ((!lsadj) && fctdrp > 0)
        ptr.posffc = std::max(ptr.posfob, ptr.posffc - fctdrp);
}

// forcst.f -- forecast/backcast the seasonals from Ie+1..Ke (and Ib-1..Ib-l),
// Iorder-order differences, weight Wt, inter-difference ratio R. Pure numeric.
void forcst(double* sts, int ib, int ie, int ke, int nyr, int iorder, double wt,
            double r) {
    auto STS = [&](int i) -> double& { return sts[i - 1]; };

    double w;
    if (!dpeq(r, 1.0)) {
        w = wt * (r - 1.0) / (dpow_ri(r, iorder) - 1.0);
    } else {
        w = wt;
    }
    int l = ke - ie;
    for (int i = 1; i <= l; ++i) {
        int j = ie + i;
        STS(j) = STS(j - nyr);
        for (int k = 1; k <= iorder; ++k)
            STS(j) = STS(j) + w * dpow_ri(r, iorder - k) *
                                  (STS(j - k * nyr) - STS(j - (k + 1) * nyr));
    }
    // Backcast from Ib-1 to Ib-l.
    if (ib > 1) {
        for (int i = 1; i <= l; ++i) {
            int j = ib - i;
            STS(j) = STS(j + nyr);
            for (int k = 1; k <= iorder; ++k)
                STS(j) = STS(j) + w * dpow_ri(r, iorder - k) *
                                      (STS(j + k * nyr) - STS(j + (k + 1) * nyr));
        }
    }
}

// vtc.f -- variable trend cycle. Selects the Henderson trend-filter length from
// the measured I/C ratio (Ratic) after a first (Ny+1)-term pass, then re-filters.
void vtc(X13Context& ctx, double* stc, double* stci) {
    x11opt_cmn& opt = ctx.x11opt;
    x11ptr_cmn& ptr = ctx.x11ptr;

    const int pos1bk = ptr.pos1bk;
    const int posffc = ptr.posffc;
    const int posfob = ptr.posfob;
    const int muladd = opt.muladd;
    const int ny = opt.ny;
    const int ktcopt = opt.ktcopt;
    const int kpart = opt.kpart;
    const bool tru7hn = ctx.x11msc.tru7hn;

    // Nterm and Tic are COMMON (x11opt) -- every assignment writes them back, and
    // hndtrn updates Tic in place in the 7-term reduce; bind by reference.
    int& nterm = opt.nterm;
    double& tic = opt.tic;

    double temp[PLEN];  // COMMON /work/ Temp(PLEN)

    // 1-based views to keep the arithmetic diffable against the Fortran.
    auto STC = [&](int i) -> double& { return stc[i - 1]; };
    auto TMP = [&](int i) -> double& { return temp[i - 1]; };

    bool lsame = false;

    // If the series is monthly apply a 13-term Henderson; if quarterly a 5-term.
    nterm = ny + 1;
    hndtrn(stc, stci, pos1bk, posffc, nterm, tic, /*lend=*/false, lsame, tru7hn);

    // Drop end terms and calculate the irregular series.
    int ib = pos1bk + nterm / 2;
    int ie = posffc - nterm / 2;
    divsub(temp, stci, stc, ib, ie, muladd);

    // Calculate the Ibar/Cbar ratio.
    int ie1 = posfob - nterm / 2 - 1;
    double apcc = 0.0;
    double apci = 0.0;
    if (muladd == 0) {
        for (int i = ib; i <= ie1; ++i) {
            apcc += std::fabs(STC(i + 1) - STC(i)) / STC(i);
            apci += std::fabs(TMP(i + 1) - TMP(i)) / TMP(i);
        }
    } else {
        for (int i = ib; i <= ie1; ++i) {
            apcc += std::fabs(STC(i + 1) - STC(i));
            apci += std::fabs(TMP(i + 1) - TMP(i));
        }
    }

    double r;
    if (dpeq(apcc, 0.0)) {
        opt.ratic = 999.0;
        r = opt.ratic;
    } else {
        opt.ratic = apci / apcc;
        r = opt.ratic * 12 / ny;
    }

    // Check if the trend-cycle moving average was preselected.
    if (ktcopt <= 0) {
        if ((kpart == 2 && r >= 1.0) || (r >= 1.0 && r < 3.5)) {
            lsame = true;
        } else if (r < 1.0) {
            if (ny == 12) {
                nterm = 9;
                tic = 1.0;
            }
        } else {
            tic = 4.5;
            nterm = 23;
            if (ny == 4) nterm = 7;
        }
    } else if (ktcopt == nterm) {
        lsame = true;
    } else {
        nterm = ktcopt;
    }

    // Generate and apply the symmetric Henderson filter and its end weights.
    hndtrn(stc, stci, pos1bk, posffc, nterm, tic, /*lend=*/true, lsame, tru7hn);
}

// sfmsr.f -- MSR global seasonal-filter selection + the vsfa/vsfb seasonal pass.
void sfmsr(X13Context& ctx, double* sts, double* stsi, int lfda, int llda,
           int lldaf) {
    x11opt_cmn& opt = ctx.x11opt;
    const int ny = opt.ny;
    const int muladd = opt.muladd;
    const bool psuadd = ctx.x11msc.psuadd;
    const bool shrtsf = ctx.x11msc.shrtsf;

    // If the MSR seasonal-filter selection option is on, calculate a global MSR
    // and try to select a seasonal filter length.
    if (opt.lterm == 6) {
        if (opt.lmsr == 6) {
            // Set llda1 to the end of the last whole year.
            int llda1 = llda - (llda % ny);
            // (The Fortran pass counter `i` fed only the deferred WRITE output.)
            while (opt.lterm == 6) {
                // If the span to be tested is less than 5 years long, use a 3x5
                // seasonal filter.
                if ((llda1 - lfda + 1) < (5 * ny)) {
                    opt.lterm = 2;
                } else {
                    vsfa(stsi, lfda, llda1, ny, muladd, psuadd, opt.rati.data(),
                         opt.ratis);
                    if (opt.ratis <= 2.5) {
                        opt.lterm = 1;
                    } else if (opt.ratis >= 6.5) {
                        opt.lterm = 3;
                    } else if (opt.ratis >= 3.5 && opt.ratis <= 5.5) {
                        opt.lterm = 2;
                        ctx.work2.l3x5 = true;
                    } else {
                        // Global MSR meets none of the criteria: drop a year from
                        // the end of the series and try again.
                        llda1 = llda1 - ny;
                    }
                }
            }
            for (int i = 1; i <= ny; ++i) {
                if (opt.lter(i) == 6) opt.lter(i) = opt.lterm;
                if (ctx.work2.l3x5 && (opt.lter(i) != 0 && opt.lter(i) != 2))
                    ctx.work2.l3x5 = false;
            }
        } else {
            // Sliding-spans run: reset the seasonal filter length to the
            // selection made for the entire series.
            opt.lterm = opt.lmsr;
            for (int i = 1; i <= ny; ++i)
                if (opt.lter(i) == 6) opt.lter(i) = opt.lmsr;
        }
    }

    double temp[PLEN];  // vsfb work scratch
    vsfa(stsi, lfda, llda, ny, muladd, psuadd, opt.rati.data(), opt.ratis);
    vsfb(sts, stsi, lfda, lldaf, ny, opt.lterm, opt.lter.data(), opt.ksect,
         shrtsf, temp, muladd);
}

// si.f -- calculates the seasonals from the SI estimates for Part B. Optional
// seasonal-MA pass (vsfa/vsfb) unless full-seasonal (Kfulsm), forms the
// irregular (Sti = Stsi/Sts, or the pseudo-additive / full-sum special cases),
// runs the sigma-limit auto-select (vtest/entsch) + extreme-value pass (xtrm),
// re-weights the SI (replac), and re-derives the seasonal (vsfb). All table/punch
// print/save is deferred (dropped), so Lfatal never trips here.
void si(X13Context& ctx, int ksect, int kfda, int klda, int nyr, int iforc,
        int nbcst, int kersa1, int ksdev1, int lfd1, int lld1, int kfulsm,
        int kfdax, int kldax) {
    x11opt_cmn& opt = ctx.x11opt;
    xtrm_cmn& xt = ctx.xtrm;
    const int ny = opt.ny;
    const int muladd = opt.muladd;
    const bool psuadd = ctx.x11msc.psuadd;
    const bool shrtsf = ctx.x11msc.shrtsf;

    double* sts = ctx.x11srs.sts.data();
    double* stsi = ctx.x11srs.stsi.data();
    double* sti = ctx.x11srs.sti.data();

    // 1-based views for the pseudo-additive irregular loop.
    auto STS = [&](int i) -> double& { return sts[i - 1]; };
    auto STSI = [&](int i) -> double& { return stsi[i - 1]; };
    auto STI = [&](int i) -> double& { return sti[i - 1]; };

    // lfd1/lld1/nbcst fed only the deferred table ranges; kept in the signature
    // to match the Fortran caller (x11pt2) once the spine wires si.
    (void)lfd1;
    (void)lld1;
    (void)nbcst;

    int llda = klda;
    if (iforc != 0 && ksect == 1) llda = klda - iforc;

    double temp[PLEN];  // COMMON /work/ Temp -- vsfb scratch + replac output

    if (kfulsm < 2) {
        if (ksect == 2)
            vsfa(stsi, kfda, llda, nyr, muladd, psuadd, opt.rati.data(),
                 opt.ratis);
        vsfb(sts, stsi, kfda, klda, nyr, opt.lterm, opt.lter.data(), opt.ksect,
             shrtsf, temp, muladd);
    }

    // (deferred: table/punch of Stsi -- B3/B8)

    if (kfulsm == 2) {
        copy(stsi, klda, 1, sti);
    } else if (psuadd) {
        for (int i = kfda; i <= klda; ++i) STI(i) = STSI(i) - STS(i) + 1.0;
    } else {
        divsub(sti, stsi, sts, kfda, klda, muladd);
    }

    if (ksect == 1 && xt.ksdev < 4) {
        int iv = 0;
        vtest(sti, iv, kfdax, kldax, ny, muladd);
        entsch(kersa1, ksdev1, xt.kersa, xt.ksdev, iv);
    }
    xtrm(sti, kfda, klda, kfdax, kldax, ny, muladd, xt.ksdev, opt.imad,
         opt.sigmu, opt.sigml, ctx.lzero.lsp, xt.stwt.data(), xt.stdper.data(),
         xt.stdev.data(), xt.csigvc.data());
    replac(stsi, temp, xt.stwt.data(), kfda, klda, nyr);

    // (deferred: table of Temp -- B4/B9)

    if (kfulsm < 2)
        vsfb(sts, stsi, kfda, klda, nyr, opt.lterm, opt.lter.data(), opt.ksect,
             shrtsf, temp, muladd);
}

// adjreg.f -- build the X-11 input buffers from the forecast/backcast-extended,
// transformed model series orix. Subtracts each regression effect (TD/holiday/
// outlier/user/seasonal/cycle) from the extended series, inverse-transforms every
// component back to the original scale, then routes the results into the X-11-
// indexed buffers: orixa -> Stcsi (the B1 decomposition input), the forecast/
// backcast tails -> Series, the calendar-adjusted series -> Stocal, and each
// active adjustment factor -> its Fac* buffer. orixmv/orixot are returned (missing-
// value- and outlier-adjusted extended series) for later X-11 stages. n is an
// output (Nrxy, or Nrxy+Sp when there are no forecasts). On the airline base path
// every factor array is zero and Kfmt==0, so orixa==orixmv==orixot==orixcl==orix
// and this reduces to invfcn + the Stcsi/Series/Stocal copies.
void adjreg(X13Context& ctx, double* orix, double* orixmv, double* orixot,
            double* ftd, double* fao, double* fls, double* ftc, double* fso,
            double* fsea, double* fcyc, double* fusr, double* fmv, double* fhol,
            int fcntyp, double lam, int nrxy, int& n) {
    const x11ptr_cmn& ptr = ctx.x11ptr;
    const extend_cmn& ext = ctx.extend;
    const x11adj_cmn& adj = ctx.x11adj;
    const x11log_cmn& xl = ctx.x11log;
    x11fac_cmn& fac = ctx.x11fac;
    orisrs_cmn& os = ctx.orisrs;
    const int pos1bk = ptr.pos1bk;
    const int posfob = ptr.posfob;
    const int posffc = ptr.posffc;
    const int kfmt = ctx.prior.kfmt;
    const int muladd = ctx.x11opt.muladd;  // Kfmt>0 addmul mode (off for airline)

    double orixa[PLEN];
    double orixcl[PLEN];
    setdp(0.0, PLEN, orixa);
    setdp(0.0, PLEN, orixmv);
    setdp(0.0, PLEN, orixot);
    setdp(0.0, PLEN, orixcl);

    // Factor arrays Ftd..Fhol are indexed 1..Nrxy; the extended series is offset
    // to the padded buffer by Pos1bk-1.
    for (int i = 1; i <= nrxy; ++i) {
        const int j = i + pos1bk - 1;
        orixa[j - 1] = orix[j - 1] - ftd[i - 1] - fls[i - 1] - fhol[i - 1] -
                       fao[i - 1] - ftc[i - 1] - fusr[i - 1] - fmv[i - 1] -
                       fsea[i - 1] - fso[i - 1] - fcyc[i - 1];
        orixmv[j - 1] = orix[j - 1] - fmv[i - 1];
        orixot[j - 1] =
            orixmv[j - 1] - fao[i - 1] - fls[i - 1] - ftc[i - 1] - fso[i - 1];
        orixcl[j - 1] = orixmv[j - 1] - ftd[i - 1] - fhol[i - 1];
    }

    // Inverse-transform the extended series/components back to the original scale.
    invfcn(ctx, orix + (pos1bk - 1), nrxy, fcntyp, lam, orix + (pos1bk - 1));
    invfcn(ctx, orixa + (pos1bk - 1), nrxy, fcntyp, lam, orixa + (pos1bk - 1));
    invfcn(ctx, orixmv + (pos1bk - 1), nrxy, fcntyp, lam, orixmv + (pos1bk - 1));
    invfcn(ctx, orixot + (pos1bk - 1), nrxy, fcntyp, lam, orixot + (pos1bk - 1));
    invfcn(ctx, orixcl + (pos1bk - 1), nrxy, fcntyp, lam, orixcl + (pos1bk - 1));

    n = (posfob == posffc) ? nrxy + ctx.model.sp : nrxy;
    const bool goodlm = dpeq(lam, 0.0) || dpeq(lam, 1.0);
    if (goodlm) {
        invfcn(ctx, ftd, n, fcntyp, lam, ftd);
        invfcn(ctx, fhol, n, fcntyp, lam, fhol);
        invfcn(ctx, fls, n, fcntyp, lam, fls);
        invfcn(ctx, ftc, n, fcntyp, lam, ftc);
        invfcn(ctx, fao, n, fcntyp, lam, fao);
        invfcn(ctx, fso, n, fcntyp, lam, fso);
        invfcn(ctx, fsea, n, fcntyp, lam, fsea);
        invfcn(ctx, fusr, n, fcntyp, lam, fusr);
        invfcn(ctx, fcyc, n, fcntyp, lam, fcyc);
    }

    // Route the adjusted series + factors into the X-11-indexed buffers.
    double* stcsi = os.stcsi.data();
    double* stocal = os.stocal.data();
    double* series = ctx.inpt.series.data();
    double* sprior = ctx.inpt.sprior.data();

    copy(orixa + (pos1bk - 1), nrxy, 1, stcsi + (pos1bk - 1));
    if (ext.nbcst > 0) {
        copy(orix + (pos1bk - 1), ext.nbcst, 1, series + (pos1bk - 1));
        if (kfmt > 0)
            addmul(series, series, sprior, pos1bk, pos1bk + ext.nbcst - 1, muladd);
    }
    if (ext.nfcst > 0) {
        copy(orix + posfob, ext.nfcst, 1, series + posfob);  // orix(Posfob+1)->Series
        if (kfmt > 0) addmul(series, series, sprior, posfob + 1, posffc, muladd);
    }
    if (goodlm) {
        if (!xl.axrgtd && adj.adjtd == 1)
            copy(ftd, n, 1, fac.factd.data() + (pos1bk - 1));
        if (adj.adjhol == 1) copy(fhol, n, 1, fac.fachol.data() + (pos1bk - 1));
        if (adj.adjao == 1) copy(fao, n, 1, fac.facao.data() + (pos1bk - 1));
        if (adj.adjls == 1) copy(fls, n, 1, fac.facls.data() + (pos1bk - 1));
        if (adj.adjtc == 1) copy(ftc, n, 1, fac.factc.data() + (pos1bk - 1));
        if (adj.adjso == 1) copy(fso, n, 1, fac.facso.data() + (pos1bk - 1));
        if (adj.adjsea == 1) copy(fsea, n, 1, fac.facsea.data() + (pos1bk - 1));
        if (adj.adjusr == 1) copy(fusr, n, 1, fac.facusr.data() + (pos1bk - 1));
        if (adj.adjcyc == 1) copy(fcyc, n, 1, fac.faccyc.data() + (pos1bk - 1));
    }

    // 'Extra' backcast factors (Nbcst2 beyond Nbcst) -> the mode identity.
    if (ext.nbcst2 > ext.nbcst) {
        const double idv = (fcntyp == 1) ? 1.0 : 0.0;
        for (int i = 1; i <= pos1bk - 1; ++i) {
            if (!xl.axrgtd && adj.adjtd == 1) fac.factd(i) = idv;
            if (!xl.axrghl && adj.adjhol == 1) fac.fachol(i) = idv;
            if (adj.adjao == 1) fac.facao(i) = idv;
            if (adj.adjls == 1) fac.facls(i) = idv;
            if (adj.adjtc == 1) fac.factc(i) = idv;
            if (adj.adjso == 1) fac.facso(i) = idv;
            if (adj.adjsea == 1) fac.facsea(i) = idv;
            if (adj.adjusr == 1) fac.facusr(i) = idv;
            if (adj.adjcyc == 1) fac.faccyc(i) = idv;
        }
    }

    // Outlier / missing-value / calendar series back to the original scale.
    if (kfmt > 0) {
        addmul(orixmv, orixmv, sprior, pos1bk, posffc, muladd);
        addmul(orixot, orixot, sprior, pos1bk, posffc, muladd);
        addmul(orixcl, orixcl, sprior, pos1bk, posffc, muladd);
    }

    // Calendar-adjusted series (with forecasts) -> Stocal.
    copy(orixcl + (pos1bk - 1), nrxy, 1, stocal + (pos1bk - 1));
}

// chkadj.f -- update the Adj*/Fin* regression-adjustment indicators from the
// regressor types actually present in the model, and count each type into
// ctx.x11adj (Ntd out; Nhol/Neas/Nao/Nls/Ntc/... on ctx). Determines which
// regression effects adjreg/prtref remove from the X-11 input. The non-log/non-
// identity WARNING WRITE is deferred (dropped).
void chkadj(X13Context& ctx, int& ntd, int khol, bool lseats, double lam) {
    using namespace prm;
    x11adj_cmn& adj = ctx.x11adj;
    const model_cmn& m = ctx.model;
    const x11log_cmn& xl = ctx.x11log;

    if (adj.adjhol < 0 && adj.finhol) adj.finhol = false;
    if (adj.adjusr < 0 && adj.finusr) adj.finusr = false;
    if (adj.adjao < 0 && adj.finao) adj.finao = false;
    if (adj.adjls < 0 && adj.finls) adj.finls = false;
    if (adj.adjtc < 0 && adj.fintc) adj.fintc = false;

    if (!(adj.adjtd >= 0 || adj.adjhol >= 0 || adj.adjao >= 0 || adj.adjls >= 0 ||
          adj.adjtc >= 0 || adj.adjso >= 0 || adj.adjsea >= 0 || adj.adjcyc >= 0 ||
          adj.adjusr >= 0 || adj.finhol || adj.finao || adj.finls || adj.fintc ||
          adj.finusr))
        return;

    int nusr = 0, nsea = 0, ncyc = 0, iusr = 1;
    ntd = 0;
    adj.nao = 0; adj.nls = 0; adj.ntc = 0; adj.nso = 0; adj.nramp = 0;
    adj.nflwtd = 0; adj.nln = 0; adj.nsln = 0; adj.nlp = 0; adj.nhol = 0;
    adj.neas = 0; adj.nseq = 0;

    for (int icol = 1; icol <= m.nb; ++icol) {
        int rtype = m.rgvrtp(icol);
        if (adj.nusrrg > 0) {
            if (rtype == PRGTUD) { rtype = ctx.usrreg.usrtyp(iusr); ++iusr; }
            else if ((rtype >= PRGTUH && rtype <= PRGUH5) || rtype == PRGTUS)
                ++iusr;
        }
        // Trading-day (flow/stock TD + length-of-month/quarter + leap year).
        if ((rtype == PRGTTD || rtype == PRGTST || rtype == PRRTTD ||
             rtype == PRRTST || rtype == PRATTD || rtype == PRATST ||
             rtype == PRG1TD || rtype == PRR1TD || rtype == PRA1TD ||
             rtype == PRG1ST || rtype == PRR1ST || rtype == PRA1ST) ||
            (rtype == PRGTLM || rtype == PRGTSL || rtype == PRGTLQ ||
             rtype == PRGTLY || rtype == PRRTLM || rtype == PRRTSL ||
             rtype == PRRTLQ || rtype == PRRTLY || rtype == PRATLM ||
             rtype == PRATSL || rtype == PRATLQ || rtype == PRATLY) ||
            (rtype == PRGUTD || rtype == PRGULM || rtype == PRGULQ ||
             rtype == PRGULY)) {
            ++ntd;
            if (rtype == PRGTTD || rtype == PRRTTD || rtype == PRATTD ||
                rtype == PRG1TD || rtype == PRR1TD || rtype == PRA1TD ||
                (m.isrflw == 0 && rtype == PRGUTD))
                ++adj.nflwtd;
            if (rtype == PRGTLM || rtype == PRGTLQ || rtype == PRRTLM ||
                rtype == PRRTLQ || rtype == PRATLM || rtype == PRATLQ ||
                rtype == PRGULM || rtype == PRGULQ)
                ++adj.nln;
            if (rtype == PRGTSL || rtype == PRRTSL || rtype == PRATSL) ++adj.nsln;
            if (rtype == PRGTLY || rtype == PRRTLY || rtype == PRATLY ||
                rtype == PRGULY)
                ++adj.nlp;
        }
        // Holiday.
        if (rtype == PRGTEA || rtype == PRGTLD || rtype == PRGTTH ||
            rtype == PRGTEC || rtype == PRGTES ||
            (rtype >= PRGTUH && rtype <= PRGUH5)) {
            ++adj.nhol;
            if (rtype == PRGTEA || rtype == PRGTEC || rtype == PRGTES)
                ++adj.neas;
        }
        // User-defined.
        if (rtype == PRGTUD) ++nusr;
        // Seasonal.
        if (rtype == PRGTUS ||
            (lseats && (rtype == PRGTSE || rtype == PRGTTS || rtype == PRRTSE ||
                        rtype == PRRTTS || rtype == PRATSE || rtype == PRATTS)))
            ++nsea;
        // AO outlier.
        if (rtype == PRGTAO || rtype == PRGUAO || rtype == PRGTAA) ++adj.nao;
        // LS / ramp.
        if (rtype == PRGTLS || rtype == PRGULS || rtype == PRGTRP ||
            rtype == PRGTAL || rtype == PRGTTL || rtype == PRGTQD ||
            rtype == PRGTQI) {
            ++adj.nls;
            if (rtype == PRGTRP || rtype == PRGTQI || rtype == PRGTQD) ++adj.nramp;
        }
        // TC outlier.
        if (rtype == PRGTTC || rtype == PRGTAT) ++adj.ntc;
        // SO outlier.
        if (rtype == PRGTSO || rtype == PRGUSO) ++adj.nso;
        // MV outlier (counted with AO).
        if (rtype == PRGTMV) ++adj.nao;
        // Transitory (cycle).
        if (rtype == PRGCYC) ++ncyc;
    }

    // Reset the adjustment indicators to match the effects actually present.
    if (adj.adjtd == 1 && ntd == 0) adj.adjtd = 0;
    if (adj.adjtd == 0 && ntd > 0) adj.adjtd = 1;
    if (adj.adjhol == 1 && adj.nhol == 0) {
        adj.adjhol = 0;
        if (!(xl.axrghl || xl.axruhl || khol >= 1) && adj.finhol) adj.finhol = false;
    }
    if (adj.adjhol == 0 && adj.nhol > 0) adj.adjhol = 1;
    if (adj.adjsea == 1 && nsea == 0) adj.adjsea = 0;
    if (adj.adjsea == 0 && nsea > 0) adj.adjsea = 1;
    if (nusr == 0) {
        if (adj.adjusr == 1) adj.adjusr = 0;
        if (adj.finusr) adj.finusr = false;
    } else if (nusr > 0) {
        if (adj.adjusr == 0) adj.adjusr = 1;
    }
    if (adj.nao == 0) {
        if (adj.adjao == 1) adj.adjao = 0;
        if (adj.finao) adj.finao = false;
    } else if (adj.nao > 0) {
        if (adj.adjao == 0) adj.adjao = 1;
    }
    if (adj.nls == 0) {
        if (adj.adjls == 1) adj.adjls = 0;
        if (adj.finls) adj.finls = false;
    } else if (adj.nls > 0) {
        if (adj.adjls == 0) adj.adjls = 1;
    }
    if (adj.ntc == 0) {
        if (adj.adjtc == 1) adj.adjtc = 0;
        if (adj.fintc) adj.fintc = false;
    } else if (adj.ntc > 0) {
        if (adj.adjtc == 0) adj.adjtc = 1;
    }
    if (adj.adjso == 1 && adj.nso == 0) adj.adjso = 0;
    if (adj.adjso == 0 && adj.nso > 0) adj.adjso = 1;
    if (adj.adjcyc == 1 && ncyc == 0) adj.adjcyc = 0;
    if (adj.adjcyc == 0 && ncyc > 0) adj.adjcyc = 1;

    // regARIMA preadjustment factors are only produced for log/no transform.
    if (!(dpeq(lam, 0.0) || dpeq(lam, 1.0))) {
        if (adj.adjtd == 1) adj.adjtd = 0;
        if (adj.adjhol == 1) adj.adjhol = 0;
        if (adj.adjao == 1) adj.adjao = 0;
        if (adj.adjls == 1) adj.adjls = 0;
        if (adj.adjtc == 1) adj.adjtc = 0;
        if (adj.adjusr == 1) adj.adjusr = 0;
        if (adj.adjsea == 1) adj.adjsea = 0;
        if (adj.adjso == 1) adj.adjso = 0;
        if (adj.adjcyc == 1) adj.adjcyc = 0;
        if (!(xl.axrghl || xl.axruhl || khol >= 1) && adj.finhol) adj.finhol = false;
        if (adj.finao) adj.finao = false;
        if (adj.finls) adj.finls = false;
        if (adj.fintc) adj.fintc = false;
        if (adj.finusr) adj.finusr = false;
    }
}

}  // namespace x13
