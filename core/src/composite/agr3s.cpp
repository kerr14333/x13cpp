// agr3s.cpp -- agr3s.f, the SEATS branch of the composite INDIRECT adjustment.
//
// x11ari.f:338-343 picks between two indirect adjustments by `X11agr` -- "every
// component was adjusted by X-11". One SEATS component turns it off and the
// total comes here instead of to agr3(). The two routines answer the same
// question with very different machinery:
//
//   agr3  re-runs the X-11 D-tables over the aggregated component results: an
//         extreme-value pass, a forced 13/5-term Henderson trend, the D8/D9 SI
//         battery, and (via x11ari.f:341's second x11pt4) the whole Part-E and
//         F2/F3 diagnostics block.
//   agr3s does none of that. The indirect seasonally adjusted series simply IS
//         the aggregate of the components' own SA series (`Ci`), and the
//         indirect seasonal factor is recovered from it by division. There is
//         no indirect trend and no indirect irregular, so `itn`, `iir`, the D8/
//         D9 pair and the entire if2./if3. block do not exist on this path --
//         measured on the oracle, which writes isf/isa/ie5/ip5/ie6/ip6/i18 and
//         nothing else.
//
// CB-32 lives here: agr3s omits agr3.f:101-108's store of the DIRECT seasonally
// adjusted series into the Orig2-aliased scratch, so agr2's Iagr==4 comparison
// measures the roughness of the aggregate ORIGINAL against the indirect SA. See
// the note at the top of the routine.
#include "composite/agr3s.hpp"

#include "common/x13context.hpp"
#include "gen/notset.hpp"
#include "numeric/numeric.hpp"
#include "specparse/specparse.hpp"   // addate, copy, setdp, writln
#include "x11/x11filt.hpp"           // divsub, change
#include "x11/x11force.hpp"          // qmap, qmap2, rndsa
#include "x11/x11tests.hpp"          // ftest

#include <vector>

namespace x13 {

namespace {
constexpr int PLEN = 1020;
}  // namespace

void agr3s(X13Context& ctx, const int* begspn, bool lx11) {
    agr_cmn& ag = ctx.agr;
    agrsrs_cmn& as = ctx.agrsrs;
    x11ptr_cmn& ptr = ctx.x11ptr;
    extend_cmn& ext = ctx.extend;
    x11opt_cmn& opt = ctx.x11opt;
    force_cmn& frc = ctx.force;
    const int ny = opt.ny;
    const int muladd = opt.muladd;
    const double rinit = (muladd == 1) ? 0.0 : 1.0;

    // agr3s.f:57-65 -- the punch range for the seasonal factors and the final
    // adjustment ratios: widened to the forecast span by x11{appendfcst} and to
    // the backcast span by appendbcst, exactly as x11pt3 does for d10.
    int lastsf;
    if (!ctx.tbllog.savfct) lastsf = ptr.posfob;
    else if (ext.nfcst > 0)  lastsf = ptr.posffc;
    else                     lastsf = ptr.posfob + ny;
    const int frstsf = ctx.tbllog.savbct ? ptr.pos1bk : ptr.pos1ob;

    ag.iagr = 4;

    // agr3s.f:99-109 -- stash the DIRECT trend in Tem. **CB-32**: agr3.f:101-108
    // ALSO stores the direct seasonally adjusted series into `tempo`, which is
    // EQUIVALENCEd onto Orig2 and is what agr2's Iagr==4 branch reads back as the
    // direct series; agr3s has no such store, so Orig2 still holds the aggregate
    // ORIGINAL (editor.f:2492 / arima.f:1432, extended with the forecasts) and
    // the R1 "DIRECT" column measures the roughness of an UNADJUSTED series.
    // Measured on the oracle: 66.777 here against 53.586 for the same data down
    // the agr3 path, and 66.777 is exactly R1 of the summed input .dat files.
    // Not reproduced by adding the store -- reproduced by leaving it out.
    //
    // The Stci write below is dead either way: :139-142 overwrites it with Ci
    // unconditionally. Transcribed because the two branches disagree about it
    // (Ci vs the DIRECT Seatsa), which is only visible if you write it down.
    for (int i = ptr.pos1ob; i <= ptr.posffc; ++i) {
        if (lx11) {
            as.tem(i) = ctx.x11srs.stc(i);
            ctx.x11srs.stci(i) = as.ci(i);
        } else {
            as.tem(i) = ctx.seatcm.seattr(i);
            ctx.x11srs.stci(i) = ctx.seatcm.seatsa(i);
        }
    }

    // agr3s.f:114-122 -- swap the run onto the INDIRECT geometry. (Nbfpob reads
    // the Nofpob line above it has just rewritten; agr3.f does the same.)
    ptr.pos1bk = ag.ind1bk;
    ptr.posffc = ag.indffc;
    ext.nofpob = ext.nofpob - ext.nfcst + ag.indnfc;
    ext.nbfpob = ext.nofpob - ext.nfcst + ag.indnfc - ext.nbcst + ag.indnbc;
    ag.dirnfc = ext.nfcst;
    ag.dirnbc = ext.nbcst;
    ext.nfcst = ag.indnfc;
    ext.nbcst = ag.indnbc;
    addate(begspn, ny, -ext.nbcst, ext.begbak.data());

    opt.kpart = 1;
    ctx.hiddn.ixreg = 0;
    opt.kswv = 0;
    if (opt.tmpma == 2) opt.tmpma = 0;

    const int pos1ob = ptr.pos1ob, posfob = ptr.posfob;
    const int pos1bk = ptr.pos1bk, posffc = ptr.posffc;

    // agr3s.f:139-142 -- the indirect SA and modified original ARE the sums.
    // No extreme-value pass: agr3's stexx/Stcime detour has no counterpart here.
    for (int i = pos1ob; i <= posffc; ++i) {
        ctx.x11srs.stci(i) = as.ci(i);
        ctx.adxser.stome(i) = as.omod(i);
    }

    // agr3s.f:147-153 -- the indirect level-shift and AO factors. Two faithful
    // asymmetries against agr3.f:198-206: there is no `Lindot` guard here, and
    // the divide that takes the level shift back OUT of Stci is commented out in
    // the Fortran (agr3s.f:151), so the indirect SA keeps it. Both matter only
    // when a component carries an LS/AO regressor.
    std::vector<double> flsind(PLEN, rinit), faoind(PLEN, rinit);
    if (ag.lindls)
        divsub(flsind.data(), as.o.data(), as.o3.data(), pos1bk, posffc, muladd);
    if (ag.lindao)
        divsub(faoind.data(), as.o.data(), as.o4.data(), pos1bk, posffc, muladd);
    ctx.agr_ils = flsind;
    ctx.agr_iao = faoind;

    // agr3s.f:178-182 -- the indirect seasonal factors and the two adjustment-
    // factor series. `ststd` is computed and then never read again in agr3s (no
    // call punches LCPCAF on this path), so a spec asking for `ita`/`iaf` gets
    // nothing; kept because Faccal beside it IS read, by the Iftrgt==3 force
    // target below.
    opt.kpart = 4;
    divsub(ctx.x11srs.sts.data(), as.o5.data(), ctx.x11srs.stci.data(),
           pos1bk, posffc, muladd);
    ctx.agr_ststd.assign(PLEN, 0.0);
    divsub(ctx.agr_ststd.data(), as.o2.data(), ctx.x11srs.stci.data(),
           pos1bk, posffc, muladd);
    divsub(ctx.x11fac.faccal.data(), as.o2.data(), as.o5.data(), pos1bk, posffc,
           muladd);

    ctx.agr_isf_frst = frstsf;
    ctx.agr_isf_last = lastsf;

    // agr3s.f:213 -- residual seasonality in the indirect SA series (Ind==1, so
    // this is the `d11.f` / `d11.3y.f` pair over the indirect adjustment).
    ftest(ctx, ctx.x11srs.stci.data(), pos1ob, posfob, ny, 1);

    // --- agr3s.f:218-306 -- force the indirect yearly totals -----------------
    int lstfrc = posfob;
    if (frc.iyrt > 0) {
        lstfrc = frc.lfctfr ? posffc : posfob;
        if (frc.lindfr) {
            ctx.agr_indforce = 1;
            // The benchmark target (x11pt3.f:715-722's four `target=` values,
            // read off the AGGREGATED buffers here).
            std::vector<double> stbase(PLEN, 0.0);
            if (frc.iftrgt == 0) {
                copy(as.o.data(), lstfrc, 1, stbase.data());
            } else if (frc.iftrgt == 1) {
                copy(as.o5.data(), lstfrc, 1, stbase.data());
            } else {
                copy(as.o1.data(), lstfrc, 1, stbase.data());
                if (frc.iftrgt == 3)
                    divsub(stbase.data(), stbase.data(), ctx.x11fac.faccal.data(),
                           pos1ob, lstfrc, muladd);
            }
            if (frc.iyrt == 1) {
                int ib = 0, ie = 0;
                qmap(stbase.data(), ctx.x11srs.stci.data(),
                     ctx.adxser.stci2.data(), pos1ob, lstfrc, ny, ib, ie,
                     frc.begyrt);
                // agr3s.f:245-260 -- extend the last full year's correction over
                // any trailing partial year, and the first full year's over any
                // leading one. NOTE the leading loop runs `Posfob, ib-1`, i.e.
                // from the LAST observation rather than from Pos1ob: with
                // ib > Pos1ob that range is empty and the intended fix-up never
                // happens. agr3.f:465 has the identical line; transcribed.
                if (ie < lstfrc) {
                    const double tempk =
                        ctx.adxser.stci2(ie) - ctx.x11srs.stci(ie);
                    for (int i = ie + 1; i <= lstfrc; ++i)
                        ctx.adxser.stci2(i) = ctx.x11srs.stci(i) + tempk;
                }
                if (ib > pos1ob) {
                    const double tempk =
                        ctx.adxser.stci2(ib) - ctx.x11srs.stci(ib);
                    for (int i = posfob; i <= ib - 1; ++i)
                        ctx.adxser.stci2(i) = ctx.x11srs.stci(i) + tempk;
                }
            } else {
                qmap2(stbase.data(), ctx.x11srs.stci.data(),
                      ctx.adxser.stci2.data(), pos1ob, lstfrc, ny, ag.iagr,
                      frc.lamda, frc.rol, frc.mid, frc.begyrt);
            }
        } else {
            // agr3s.f:266 -- not forced here: the indirect forced series is the
            // AGGREGATE of the components' own forced SA series.
            ctx.agr_indforce = 0;
            copy(as.ci2.data(), lstfrc, 1, ctx.adxser.stci2.data());
        }

        ftest(ctx, ctx.adxser.stci2.data(), pos1ob, posfob, ny, 1);

        // agr3s.f:295 -- the forcing factor, over [Posfob, lstfrc]. That range
        // is a single point unless Lfctfr widened it; agr3.f:504 has the same
        // start, so the `iff` table's leading rows are whatever the buffer held.
        ctx.agr_frcfac.assign(PLEN, 0.0);
        divsub(ctx.agr_frcfac.data(), ctx.x11srs.stci.data(),
               ctx.adxser.stci2.data(), posfob, lstfrc, muladd);
        ctx.agr_frcfac_last = lstfrc;
    }

    // --- agr3s.f:311-338 -- the rounded indirect SA series -------------------
    if (frc.lrndsa) {
        bool rndok = false;
        rndsa(ctx.x11srs.stci.data(), ctx.adxser.stcirn.data(), pos1ob, posfob,
              ny, opt.kdec, rndok);
        if (rndok) {
            // agr3s.f:327-329 -- the residual-seasonality test on the rounded
            // series runs only with an X-11 direct adjustment. agr3.f has no such
            // guard; transcribed as written.
            if (lx11) ftest(ctx, ctx.adxser.stcirn.data(), pos1ob, posfob, ny, 1);
        } else {
            frc.lrndsa = false;
        }
    }

    opt.kpart = 5;

    // --- agr3s.f:345-410 -- the Part-E change tables --------------------------
    // agr3 gets these from x11ari.f:341's second x11pt4 pass; agr3s computes E5
    // and E6 (plus the forced/rounded twins) itself and produces no other E
    // table at all.
    const int mfda = pos1ob + 1;
    bool* gudval = ctx.goodob.gudval.data();
    ctx.x11_e5.assign(PLEN, 0.0);
    ctx.x11_e6.clear();
    ctx.x11_e6a.clear();
    ctx.x11_e6r.clear();
    change(ctx.inpt.series.data(), ctx.x11_e5.data(), mfda, posfob, muladd,
           gudval);
    if (opt.kfulsm == 0) {
        ctx.x11_e6.assign(PLEN, 0.0);
        change(ctx.x11srs.stci.data(), ctx.x11_e6.data(), mfda, posfob, muladd,
               gudval);
        // agr3s.f:378-383 -- the forced/rounded change tables start at the first
        // period of the first FORCED year, not at mfda.
        int ify = mfda;
        if (frc.iyrt > 0) {
            const int m = pos1ob % ny;
            if (m > frc.iyrt) ify = (((pos1ob + ny - 2) / ny) * ny) + frc.iyrt + 1;
            else              ify = (((pos1ob - 1) / ny) * ny) + frc.iyrt + 1;
            ctx.x11_e6a.assign(PLEN, 0.0);
            change(ctx.adxser.stci2.data(), ctx.x11_e6a.data(), ify, posfob,
                   muladd, gudval);
        }
        if (frc.lrndsa) {
            if (frc.iyrt == 0) ify = mfda;
            ctx.x11_e6r.assign(PLEN, 0.0);
            change(ctx.adxser.stcirn.data(), ctx.x11_e6r.data(), ify, posfob,
                   muladd, gudval);
        }
        ctx.agr_e6_ify = ify;
    }

    // --- agr3s.f:412-459 -- the final adjustment ratios A1/D11 (`i18`) and,
    // only when one of them could not be formed cleanly, the total adjustment
    // factors (`ita`). `pre18b` is set by a zero or negative ORIGINAL as well as
    // by a zero SA, so the second table exists precisely when the ratio is not
    // everywhere meaningful.
    const int e18_last = (posffc > posfob) ? posffc : posfob;
    std::vector<double> stcirb(PLEN, 0.0);
    bool pre18b = false;
    for (int i = pos1ob; i <= e18_last; ++i) {
        const double sa = ctx.x11srs.stci(i);
        const double y = ctx.inpt.series(i);
        if (dpeq(sa, 0.0)) {
            if (dpeq(y, 0.0)) {
                stcirb[static_cast<std::size_t>(i - 1)] = 1.0;
            } else {
                stcirb[static_cast<std::size_t>(i - 1)] = prm::DNOTST;
                pre18b = true;
            }
        } else {
            if (dpeq(y, 0.0) || y < 0.0) pre18b = true;
            stcirb[static_cast<std::size_t>(i - 1)] = y / sa;
        }
    }
    ctx.x11_e18 = stcirb;
    ctx.x11_eb.clear();
    if (pre18b) {
        std::vector<double> eb(PLEN, 0.0);
        divsub(eb.data(), ctx.inpt.series.data(), ctx.x11srs.stci.data(),
               pos1ob, e18_last, muladd);
        ctx.x11_eb = eb;
    }
    ctx.agr_e18_last = e18_last;
    ctx.agr3s_ran = true;

    // --- a MEASURED gap, made loud rather than silent ------------------------
    // Everything above that reads the indirect SA past the observed span is
    // reading zeroes. `Ci` is the aggregate of the components' `Seatsa`, and
    // seatad.f:49-54 appends `Setfsa` -- the SEATS FORECAST decomposition
    // (ansub3.f:356-678) -- into Seatsa over [Posfob+1, Posffc]. This port has
    // no Setfsa, so that append does not happen.
    //
    // The forecast span is NOT avoidable by configuration: editor.f:387-400
    // forces `Nfcst >= max(12, 3*Sp)` on any SEATS run regardless of
    // forecast{maxlead=}, so Indnfc is always positive for a SEATS component.
    // What it costs, measured on tests/corpus/census-examples/composite-seats:
    // i18 is punched to Posffc unconditionally (agr3s.f:412-418 -- unlike isf
    // there is no Savfct gate) and its forecast rows are wrong; and when the
    // TOTAL also carries forecasts, `Series` extends where `Ci` does not, so
    // `pre18b` turns on and the run emits an `ita` table the oracle does not.
    // Everything over the OBSERVED span -- isf, isa, ie5/ie6, i18, and the
    // whole agr2 comparison block -- is bit-exact (~5e-15) and gated.
    if (ag.indnfc > 0) {
        writln(ctx, " NOTE: The indirect adjustment past the end of the series "
                    "is not computed:",
               ctx.units.mt2, ctx.units.mt2, true);
        writln(ctx, "       the SEATS forecast decomposition (Setfsa) is not "
                    "implemented, so the",
               ctx.units.mt2, ctx.units.mt2, false);
        writln(ctx, "       indirect tables are reliable only over the observed "
                    "span.",
               ctx.units.mt2, ctx.units.mt2, false);
    }
}

}  // namespace x13
