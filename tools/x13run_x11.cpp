// x13run_x11.cpp -- M5 X-11 CLI harness: parse a spec, assemble the X-11
// decomposition spine (x13::run_x11, the no-model direct-X11 path), then print
// the X-11 tables as `<tag> YYYYMM <value>` lines to stdout for the X-11 parity
// gate to diff against the oracle save-table goldens.
//
// Usage: x13run_x11 <specfile.spc>
//
// Like the other run_* harnesses this writes no output file -- the library keeps
// everything on the context (the package's "no auto file output" rule); this thin
// driver only surfaces the ctx x11srs arrays to stdout, numerically. The exact
// oracle-format .b1/.d* save emission is a separate deferred milestone.
//
// Tables printed (span [Pos1ob, Posfob]):
//   b1  -- prior-adjusted B1 input          (orisrs Stcsi)
// plus, for inspection while the D-pass finals (x11pt3) land, the working arrays
// left by x11pt2 at D7: trend (Stc), seasonal (Sts), SA (Stci), irregular (Sti).
// Only b1 is a stable golden target until x11pt3/x11pt4 are wired.
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

#include "specparse/specparse.hpp"
#include "common/x13context.hpp"
#include "numeric/numeric.hpp"    // dpeq
#include "gen/notset.hpp"         // prm::DNOTST

namespace {
std::string dirname_of(const std::string& p) {
    std::size_t s = p.find_last_of("/\\");
    return (s == std::string::npos) ? std::string() : p.substr(0, s);
}
std::string basename_of(const std::string& p) {
    std::size_t s = p.find_last_of("/\\");
    return (s == std::string::npos) ? p : p.substr(s + 1);
}

// Emit one table section: `<tag> YYYYMM <value>` over the buffer range
// [first, last]. `anchor` is the buffer index that carries the date Begspn --
// always Pos1ob, which is NOT the same as `first`: x11{appendbcst=yes} widens
// the punch range back to Pos1bk, and those leading rows are dated BEFORE
// Begspn. Anchoring on `first` instead labelled every row Nbcst periods late,
// so a date-keyed diff against the oracle compared row k to row k+Nbcst and
// reported a 30-70% "drift" on values that were in fact bit-identical.
void dump(const char* tag, const int* begspn, int sp, int first, int last,
          const double* arr /*1-based*/, int anchor) {
    for (int i = first; i <= last; ++i) {
        int idate[2];
        x13::addate(begspn, sp, i - anchor, idate);
        std::printf("%s %04d%02d %.15E\n", tag, idate[0], idate[1], arr[i - 1]);
    }
}

// Emit a slidingspans{} wide table: `<tag> YYYYMM <span1> .. <spanN> <maxdiff>`,
// one line per date row (svspan.f's DO l0=Im,Sslen+Im-1), -999 sentinel for
// "this span doesn't cover this date" cells (matching the golden .sfs/.chs
// convention -- see tests/parity/test_slidingspans_tables.py). arr is a
// column-major MXLEN(276) x MXCOL(4) flat buffer (or an x13::farray2 handed
// via .data()); dmax is 1-based over MXLEN.
constexpr int MXLEN_SS = 276;
void dump_span_table(const char* tag, int iyr, int im, int nsea, int sslen,
                      int ncol, const double* arr, const double* dmax) {
    constexpr double SENTINEL = -999.0;
    int date[2] = {iyr, im};
    for (int l0 = im; l0 <= sslen + im - 1; ++l0) {
        int idate[2];
        x13::addate(date, nsea, l0 - im, idate);
        std::printf("%s %04d%02d", tag, idate[0], idate[1]);
        for (int l = 1; l <= ncol; ++l) {
            double v = arr[(l0 - 1) + (l - 1) * MXLEN_SS];
            std::printf(" %+.15E", x13::dpeq(v, x13::prm::DNOTST) ? SENTINEL : v);
        }
        double dv = dmax[l0 - 1];
        std::printf(" %+.15E\n", x13::dpeq(dv, x13::prm::DNOTST) ? SENTINEL : dv);
    }
}
}  // namespace

int main(int argc, char** argv) {
    std::setvbuf(stderr, nullptr, _IONBF, 0);  // DBG: unbuffer stderr for crash-time flush
    if (argc < 2) {
        std::fprintf(stderr, "usage: x13run_x11 <specfile.spc>\n");
        return 2;
    }
    std::string path = argv[1];
    std::string dir = dirname_of(path);
    std::string full = basename_of(path);
    std::string base = full.size() >= 4 && full.substr(full.size() - 4) == ".spc"
                           ? full.substr(0, full.size() - 4)
                           : full;

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "x13run_x11: cannot open %s\n", path.c_str());
        return 2;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string text = ss.str();
    in.close();

    if (!dir.empty()) chdir(dir.c_str());

    auto ctxp = std::make_unique<x13::X13Context>();
    x13::X13Context& ctx = *ctxp;
    bool ok;
    try {
        ok = x13::run_x11(ctx, text, base);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "x13run_x11: exception: %s\n", e.what());
        return 3;
    }

    std::printf("OUTCOME: %s\n", ok ? "OK" : "FATAL");
    if (!ok) {
        // Surface the .err channel so a FATAL names its blocking stub.
        std::fputs(ctx.channels_.unit(ctx.units.mt2).str().c_str(), stderr);
        return 1;
    }

    const int sp = ctx.model.sp;
    const int* begspn = ctx.mdldat.begspn.data();
    const int pos1ob = ctx.x11ptr.pos1ob;
    const int posfob = ctx.x11ptr.posfob;

    // Gate targets produced by the x11pt1 + x11pt2 + x11pt3 spine:
    //   b1  -- prior-adjusted B1 input. No-model path: Series == B1. Model path:
    //          run_x11 snapshots the adjreg-adjusted B1 into Stoap (x11pt2/pt3
    //          overwrite Stcsi; Series stays the ORIGINAL that x11pt3 needs).
    //   d10 -- final seasonal (Sts)     d11 -- final SA (Stci)
    //   d12 -- final trend-cycle (Stc)  d13 -- final irregular (Sti)
    // (D12 supersedes the earlier D7 trend gate -- x11pt3 recomputes Stc into D12.)
    // b1 honours series{appendfcst/appendbcst=yes} (Savfct/Savbct): forecasts
    // extend the range forward to Posffc, backcasts extend it back to Pos1bk
    // (agr3.f:158-160). Only meaningful on the model path (Stoap holds the
    // forecast/backcast-extended B1); the no-model Series has no model extension.
    const int b1_frst = (ctx.captured.has_model && ctx.tbllog.savbct)
                            ? ctx.x11ptr.pos1bk : pos1ob;
    const int b1_last = (ctx.captured.has_model && ctx.tbllog.savfct)
                            ? ctx.x11ptr.posffc : posfob;
    // B1 source: model path -> the adjreg-adjusted Stoap snapshot; no-model with
    // an X-11 Easter prior -> the prior-adjusted Stoap snapshot (x11pt1); plain
    // no-model -> the raw series (== unadjusted B1).
    const double* b1src =
        (ctx.captured.has_model || ctx.x11opt.khol > 1)
            ? ctx.orisrs.stoap.data()
            : ctx.inpt.series.data();
    dump("b1",  begspn, sp, b1_frst, b1_last, b1src, pos1ob);
    // d10 seasonal factors are projected across the forecast/backcast span, so
    // they too honour appendfcst/appendbcst (the oracle saves d10 over the same
    // extended range as b1). d11/d12/d13 (SA/trend/irregular of the data) do not.
    // x11pt3.f:197-205 (D10) / 1132-1140 (D16) -- the seasonal-factor punch
    // range. Savfct (series{}/x11{}/seats{} appendfcst) widens it forward to the
    // forecast span (Posffc when Nfcst>0, else the one projected year Posfob+Ny);
    // Savbct widens the start back to Pos1bk. Unlike b1 this is NOT model-gated:
    // the seasonal factors are projected a year ahead regardless.
    const int sf_frst = ctx.tbllog.savbct ? ctx.x11ptr.pos1bk : pos1ob;
    const int sf_last = !ctx.tbllog.savfct
                            ? posfob
                            : (ctx.extend.nfcst > 0 ? ctx.x11ptr.posffc
                                                    : posfob + sp);
    dump("d10", begspn, sp, sf_frst, sf_last, ctx.x11srs.sts.data(), pos1ob);
    dump("d11", begspn, sp, pos1ob, posfob, ctx.x11srs.stci.data(), pos1ob);
    dump("d12", begspn, sp, pos1ob, posfob, ctx.x11srs.stc.data(), pos1ob);
    dump("d13", begspn, sp, pos1ob, posfob, ctx.x11srs.sti.data(), pos1ob);
    // d16 -- combined seasonal + calendar adjustment factors (x11pt3's ststd,
    // snapshotted onto ctx). Empty when the oracle writes no D16 (Khol==1).
    if (!ctx.x11_ststd.empty())
        dump("d16", begspn, sp, sf_frst, sf_last, ctx.x11_ststd.data(), pos1ob);

    // Part-E tables (x11pt4.f:85-319). Punch ranges per prtagr.f/pragr2.f:
    //   e1/e2/e3/e11  [Pos1ob, Posfob]  (Ibsav==Ib, so appendfcst/bcst is inert)
    //   e5-e8         [Pos1ob+1, Posfob]  -- pe5-pe8 are the same series x100
    //   e18/eb        [Pos1ob, Posfob], widening to Pos1bk / Posffc under
    //                 x11{appendbcst}/{appendfcst}
    if (ctx.x11_etables_set) {
        dump("e1", begspn, sp, pos1ob, posfob, ctx.adxser.stome.data(), pos1ob);
        dump("e2", begspn, sp, pos1ob, posfob, ctx.adxser.stcime.data(), pos1ob);
        dump("e3", begspn, sp, pos1ob, posfob, ctx.mq5a_stime.data(), pos1ob);
        const int chg1 = pos1ob + 1;
        struct { const char* tag; const std::vector<double>* v; } chg[] = {
            {"e5", &ctx.x11_e5},   {"e6", &ctx.x11_e6},
            {"e6a", &ctx.x11_e6a}, {"e6r", &ctx.x11_e6r},
            {"e7", &ctx.x11_e7},   {"e8", &ctx.x11_e8},
        };
        for (const auto& c : chg) {
            if (c.v->empty()) continue;
            dump(c.tag, begspn, sp, chg1, posfob, c.v->data(), pos1ob);
            // The p<tag> twin is the SAME series; pragr2.f passes `Muladd.ne.1`
            // as punch's percent flag, so it is scaled to percent in
            // multiplicative / log-additive mode and printed unscaled in
            // additive mode (where the "changes" are already differences).
            const double pscale = (ctx.x11opt.muladd != 1) ? 100.0 : 1.0;
            for (int i = chg1; i <= posfob; ++i) {
                int idate[2];
                x13::addate(begspn, sp, i - pos1ob, idate);
                std::printf("p%s %04d%02d %.15E\n", c.tag, idate[0], idate[1],
                            (*c.v)[i - 1] * pscale);
            }
        }
        dump("e11", begspn, sp, pos1ob, posfob, ctx.x11_e11.data(), pos1ob);
        const int e18_frst = ctx.tbllog.savbct ? ctx.x11ptr.pos1bk : pos1ob;
        const int e18_last = ctx.tbllog.savfct ? ctx.x11ptr.posffc : posfob;
        dump("e18", begspn, sp, e18_frst, e18_last, ctx.x11_e18.data(), pos1ob);
        dump("eb", begspn, sp, e18_frst, e18_last, ctx.x11_eb.data(), pos1ob);
    }

    // transform{constant=}: the D11 / published D12 with the constant still in,
    // i.e. the oracle's Stcipc / stc2pc (save tokens sac / tac).
    if (!ctx.x11_stcipc.empty())
        dump("sac", begspn, sp, pos1ob, posfob, ctx.x11_stcipc.data(), pos1ob);
    if (!ctx.x11_stc2pc.empty())
        dump("tac", begspn, sp, pos1ob, posfob, ctx.x11_stc2pc.data(), pos1ob);

    // F2 seasonality tests (svf2f3.f:59-64). These are savelog canaries, not
    // tables: emitted here at the oracle's own printed precision so the gate can
    // read the .udg golden directly. Fpres/P3 come from the B1 F-test in x11pt2,
    // the rest from the D8 battery in x11pt3.
    if (ctx.x11_f2tests_set) {
        const x13::tests_cmn& t = ctx.x11_f2tests;
        std::printf("f2.fsb1 %.3f %.2f\n", t.fpres, t.p3);
        std::printf("f2.fsd8 %.3f %.2f\n", t.fstabl, t.p1);
        std::printf("f2.kw %.3f %.2f\n", t.chikw, t.p5);
        std::printf("f2.msf %.3f %.2f\n", t.fmove, t.p2);
        std::printf("f2.idseasonal %s\n", t.iqfail == 1 ? "yes" : "no");
    }

    // The rest of the F2 summary-measure block + the F3 quality statistics
    // (svf2f3.f:28-58 and :96-113). Emitted at full precision; the gate applies
    // the .udg's own PRINTED precision as its tolerance (E15.8 / F8.2 / f6.3).
    if (ctx.x11_f3_set) {
        const x13::inpt2_cmn& q = ctx.x11_f2inpt2;
        const x13::work2_cmn& w = ctx.x11_f2work2;
        const int ny = ctx.x11opt.ny;
        for (int i = 1; i <= ny; ++i)
            std::printf("f2.a%02d %.15E %.15E %.15E %.15E %.15E %.15E %.15E "
                        "%.15E %.15E %.15E %.15E\n",
                        i, q.obar(i), q.cibar(i), q.ibar(i), q.cbar(i),
                        q.sbar(i), w.pbar(i), q.tdbar(i), q.smbar(i),
                        q.ombar(i), q.cimbar(i), q.imbar(i));
        for (int i = 1; i <= ny; ++i)
            std::printf("f2.b%02d %.15E %.15E %.15E %.15E %.15E %.15E\n", i,
                        q.isq(i), q.csq(i), q.ssq(i), w.psq(i), q.tdsq(i),
                        q.osq2(i));
        for (int i = 1; i <= ny; ++i)
            std::printf("f2.c%02d %.15E %.15E %.15E %.15E %.15E %.15E %.15E "
                        "%.15E %.15E %.15E %.15E %.15E\n",
                        i, q.obar2(i), q.osd(i), q.ibar2(i), q.isd(i),
                        q.cbar2(i), q.csd(i), q.sbar2(i), q.ssd(i), q.cibar2(i),
                        q.cisd(i), q.smbar2(i), q.smsd(i));
        std::printf("f2.d %.15E %.15E %.15E %.15E\n", q.adrci, q.adri, q.adrc,
                    q.adrmcd);
        std::printf("f2.e");
        for (int i = 1; i <= ny; ++i) std::printf(" %.15E", q.smic(i));
        std::printf("\n");
        std::printf("f2.mcd %d\n", ctx.x11_f2mcd);
        std::printf("f2.f %.15E %.15E %.15E %.15E %.15E %.15E\n", q.vi, q.vc,
                    q.vs, q.vp, q.vtd, q.rv);
        std::printf("f2.g");
        for (int i = 1; i <= ny + 2; ++i) std::printf(" %.15E", w.autoc(i));
        std::printf("\n");
        std::printf("f2.ic %.15E\n", ctx.x11_f2ratic);
        std::printf("f2.is %.15E\n", ctx.x11_f2ratis);
        // svf2f3.f:97 -- M6 is skipped from the printout when Kfulsm==2.
        for (int i = 1; i <= w.nn; ++i)
            if (i != 6 || ctx.x11opt.kfulsm < 2)
                std::printf("f3.m%02d %.15E\n", i, w.qu(i));
        std::printf("f3.q %.15E\n", w.qual);
        std::printf("f3.qm2 %.15E\n", w.q2m2);
        std::printf("f3.fail %d\n", w.kfail);
    }

    // a4 -- x11regression tdprior user prior trading-day factor (Kswv=1 pritd),
    // over the observed span [pos1ob,posfob]. x11_a4_prior is 0-based from pos1ob.
    if (!ctx.x11_a4_prior.empty()) {
        for (std::size_t k = 0; k < ctx.x11_a4_prior.size(); ++k) {
            int idate[2];
            x13::addate(begspn, sp, static_cast<int>(k), idate);
            std::printf("a4 %04d%02d %.15E\n", idate[0], idate[1],
                        ctx.x11_a4_prior[k]);
        }
    }

    // Force yearly totals (force{} spec, Iyrt>0): D11A forced SA series (saa =
    // Stci2, left by x11pt3's qmap benchmarking) and the per-obs forcing factor
    // (ffc = Stci/Stci2 mult, Stci-Stci2 add). saa is printed over [pos1ob,
    // posfob] (x11pt3.f:788); ffc over [pos1ob, lstfrc], where lstfrc extends to
    // posffc (the forecast-extended span) when usefcst is on (the default).
    if (ctx.force.iyrt > 0) {
        const double* stci = ctx.x11srs.stci.data();
        const double* stci2 = ctx.adxser.stci2.data();
        const int muladd = ctx.x11opt.muladd;
        const int lstfrc = ctx.force.lfctfr ? ctx.x11ptr.posffc : posfob;
        std::vector<double> ffc(static_cast<std::size_t>(lstfrc), 0.0);
        for (int i = pos1ob; i <= lstfrc; ++i)
            ffc[i - 1] = (muladd == 0) ? stci[i - 1] / stci2[i - 1]
                                       : stci[i - 1] - stci2[i - 1];
        dump("saa", begspn, sp, pos1ob, posfob, stci2, pos1ob);
        dump("ffc", begspn, sp, pos1ob, lstfrc, ffc.data(), pos1ob);
        // rnd: the rounded SA series (round=yes -> rndsa), over [pos1ob, posfob].
        if (ctx.force.lrndsa)
            dump("rnd", begspn, sp, pos1ob, posfob, ctx.adxser.stcirn.data(), pos1ob);
    }

    // slidingspans{} sfs (seasonal-factor spans) / chs (month-to-month SA-
    // change spans) -- see tools/slidingspans_scope.md; ads/tds are not
    // produced by this gate corpus (no TD/holiday/round/force -- ssap.f's
    // mflag gating never fires for them).
    if (ctx.ssout.ran) {
        const auto& so = ctx.ssout;
        dump_span_table("sfs", so.iyr, so.im, so.nsea, so.sslen, so.ncol,
                         ctx.sspdat.s.data(), so.dmax_sfs.data());
        dump_span_table("chs", so.iyr, so.im, so.nsea, so.sslen, so.ncol,
                         so.c_flat.data(), so.dmax_chs.data());
    }

    // spectrum{} sp0/sp1/sp2 periodogram tables -- `<tag> <pos> <freq> <value>`,
    // one line per frequency (Pos 0..60). Gated in test_spectrum_tables.py vs
    // the oracle .sp0/.sp1/.sp2 goldens.
    if (ctx.spcout.ran) {
        const auto& sc = ctx.spcout;
        auto emit_spec = [&](const char* tag, const std::vector<double>& v) {
            for (std::size_t i = 0; i < v.size(); ++i)
                std::printf("%s %zu %.15E %.15E\n", tag, i, sc.frq[i], v[i]);
        };
        if (sc.have_sp0) emit_spec("sp0", sc.sp0);
        if (sc.have_sp1) emit_spec("sp1", sc.sp1);
        if (sc.have_sp2) emit_spec("sp2", sc.sp2);
        if (sc.have_spr) emit_spec("spr", sc.spr);
        // Tukey tables use their own i/m frequency grid (sc.frq_tukey).
        auto emit_tukey = [&](const char* tag, const std::vector<double>& v) {
            for (std::size_t i = 0; i < v.size(); ++i)
                std::printf("%s %zu %.15E %.15E\n", tag, i, sc.frq_tukey[i], v[i]);
        };
        if (sc.have_st0) emit_tukey("st0", sc.st0);
        if (sc.have_st1) emit_tukey("st1", sc.st1);
        if (sc.have_st2) emit_tukey("st2", sc.st2);
    }

    // x11regression{} b16/c16 regression trading-day factors (B/C iterations),
    // over [pos1ob, posfob]. Gated in test_x11regression_tables.py.
    if (ctx.x11reg_ran) {
        auto emit16 = [&](const char* tag, const std::vector<double>& v) {
            for (int i = pos1ob; i <= posfob; ++i) {
                int idate[2];
                x13::addate(begspn, sp, i - pos1ob, idate);
                std::printf("%s %04d%02d %.15E\n", tag, idate[0], idate[1],
                            v[i - pos1ob]);
            }
        };
        if (!ctx.x11reg_b16.empty()) emit16("b16", ctx.x11reg_b16);
        if (!ctx.x11reg_c16.empty()) emit16("c16", ctx.x11reg_c16);

        // xrm design matrix: one row per data date, Nb regressor columns.
        const int nc = ctx.x11reg_xrm_ncol;
        if (nc > 0 && !ctx.x11reg_xrm.empty()) {
            const int nrows = static_cast<int>(ctx.x11reg_xrm.size()) / nc;
            for (int r = 0; r < nrows; ++r) {
                int idate[2];
                x13::addate(begspn, sp, r, idate);
                std::printf("xrm %04d%02d", idate[0], idate[1]);
                for (int c = 0; c < nc; ++c)
                    std::printf(" %.15E", ctx.x11reg_xrm[static_cast<std::size_t>(r) * nc + c]);
                std::printf("\n");
            }
        }

        // x11regression aictest=(easter) AICC canaries: one line per window
        // ("aicc_xe <window> <value>", window 0 == noeaster) + the chosen window.
        if (ctx.x11reg_xe_ran) {
            for (const auto& wa : ctx.x11reg_aicc_xe)
                std::printf("aicc_xe %d %.15E\n", wa.first, wa.second);
            std::printf("aicc_xe_window %d\n", ctx.x11reg_xe_window);
        }
    }

    // history{} sar/sae (SA revision / conc+final) and trr/tre (trend) -- one
    // line per revision-table row (see tests/parity/test_history_tables.py).
    if (ctx.hist_out.ran) {
        const auto& ho = ctx.hist_out;
        for (std::size_t r = 0; r < ho.dates.size(); ++r) {
            if (ho.have_sa) {
                std::printf("sar %06d %.15E\n", ho.dates[r], ho.sar[r]);
                std::printf("sae %06d %.15E %.15E\n", ho.dates[r],
                            ho.sae_cnc[r], ho.sae_fin[r]);
            }
            if (ho.have_tr) {
                std::printf("trr %06d %.15E\n", ho.dates[r], ho.trr[r]);
                std::printf("tre %06d %.15E %.15E\n", ho.dates[r],
                            ho.tre_cnc[r], ho.tre_fin[r]);
            }
            if (ho.have_ch) {
                std::printf("chr %06d %.15E\n", ho.dates[r], ho.chr[r]);
                std::printf("che %06d %.15E %.15E\n", ho.dates[r],
                            ho.che_cnc[r], ho.che_fin[r]);
            }
            if (ho.have_sf) {
                std::printf("sfr %06d %.15E %.15E\n", ho.dates[r],
                            ho.sfr_cnc[r], ho.sfr_proj[r]);
                std::printf("sfe %06d %.15E %.15E %.15E\n", ho.dates[r],
                            ho.sfe_cnc[r], ho.sfe_proj[r], ho.sfe_fin[r]);
            }
            if (ho.have_tch) {
                std::printf("tcr %06d %.15E\n", ho.dates[r], ho.tcr[r]);
                std::printf("tce %06d %.15E %.15E\n", ho.dates[r],
                            ho.tce_cnc[r], ho.tce_fin[r]);
            }
        }
        // fcst history: fce/fch carry nfctlg columns per row and their own date
        // range, so they are emitted from ho.fdates rather than ho.dates.
        if (ho.have_fct) {
            const int nl = ho.nfctlg;
            for (std::size_t r = 0; r < ho.fdates.size(); ++r) {
                std::printf("fce %06d", ho.fdates[r]);
                for (int k = 0; k < nl; ++k)
                    std::printf(" %.15E", ho.fce[r * nl + k]);
                std::printf("\n");
                std::printf("fch %06d", ho.fdates[r]);
                for (int k = 0; k < nl; ++k)
                    std::printf(" %.15E %.15E", ho.fch_fcst[r * nl + k],
                                ho.fch_err[r * nl + k]);
                std::printf("\n");
            }
            std::printf("rvfcstlag");
            for (int k = 0; k < nl; ++k) std::printf(" %d", ho.fctlag[k]);
            std::printf("\n");
            std::printf("meanssfe");
            for (int k = 0; k < nl; ++k) std::printf(" %.15E", ho.meanssfe[k]);
            std::printf("\n");
        }
        // aic/arma/td model histories: their own (one-longer) date range.
        for (std::size_t r = 0; r < ho.mdates.size(); ++r) {
            if (ho.have_aic)
                std::printf("lkh %06d %.15E %.15E\n", ho.mdates[r],
                            ho.lkh_lkhd[r], ho.lkh_aicc[r]);
            if (ho.have_arma) {
                std::printf("amh %06d", ho.mdates[r]);
                for (int k = 0; k < ho.nrvarma; ++k)
                    std::printf(" %.15E", ho.amh[r * ho.nrvarma + k]);
                std::printf("\n");
            }
            if (ho.have_tdrg) {
                std::printf("tdh %06d", ho.mdates[r]);
                for (int k = 0; k < ho.nrvtdrg; ++k)
                    std::printf(" %.15E", ho.tdh[r * ho.nrvtdrg + k]);
                std::printf("\n");
            }
        }
    }
    return 0;
}
