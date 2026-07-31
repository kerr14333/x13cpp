// x13run_iddiff.cpp -- focused test harness for the automatic differencing-order
// identifier (iddiff.f). Parses a spec, runs the pre-model phase (series read +
// transform + regvar, no estimation) via x13::run_m2, then drives x13::iddiff on
// the transformed series and prints the identified orders as `KEY value` lines.
//
// Usage: x13run_iddiff <specfile.spc> [maxdr] [maxds]   (defaults 2 1)
//
// The oracle reports iddiff's decision in the automdl .udg as
// `idnonseasonaldiff.first` / `idseasonaldiff.first`; the parity gate diffs the
// printed idr/ids against those. No output files are written (package rule).
#include <cstdio>
#include <cstdlib>
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

#include "automdl/iddiff.hpp"
#include "automdl/amdid.hpp"
#include "automdl/automd.hpp"
#include "automdl/amdest.hpp"   // cnvmdl (read identified orders)
#include "automdl/aictst.hpp"   // tdaic, easaic (AIC-test regressor family)
#include "automdl/mdlset.hpp"   // mdlint, mdlset (default-model construction)
#include "regarima/regvar.hpp"  // regvar
#include "specparse/specparse.hpp"
#include "gen/srslen.hpp"   // prm::PLEN
#include "gen/model.hpp"    // prm::PORDER, prm::PTDAIC/PEAIC
#include "gen/notset.hpp"   // prm::NOTSET, prm::DNOTST

namespace {
std::string dirname_of(const std::string& p) {
    std::size_t s = p.find_last_of("/\\");
    return (s == std::string::npos) ? std::string() : p.substr(0, s);
}
std::string basename_of(const std::string& p) {
    std::size_t s = p.find_last_of("/\\");
    return (s == std::string::npos) ? p : p.substr(s + 1);
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: x13run_iddiff <specfile.spc> [maxdr] [maxds]\n");
        return 2;
    }
    std::string path = argv[1];
    int maxdr = 2, maxds = 1;
    {
        int pos = 0;  // positional numeric args, skipping --flags
        for (int i = 2; i < argc; ++i) {
            std::string s = argv[i];
            if (s.rfind("--", 0) == 0) continue;
            if (pos == 0) maxdr = std::atoi(s.c_str());
            else if (pos == 1) maxds = std::atoi(s.c_str());
            ++pos;
        }
    }

    std::string dir = dirname_of(path);
    std::string full = basename_of(path);
    std::string base = full.size() >= 4 && full.substr(full.size() - 4) == ".spc"
                           ? full.substr(0, full.size() - 4)
                           : full;

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "x13run_iddiff: cannot open %s\n", path.c_str());
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
        ok = x13::run_m2(ctx, text, base, /*estimate=*/false);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "x13run_iddiff: exception: %s\n", e.what());
        return 3;
    }
    if (!ok || ctx.error.lfatal) {
        std::printf("OUTCOME: FATAL\n");
        return 1;
    }

    bool do_amdid = false, do_automd = false, do_trnaic = false;
    bool do_aictest = false, want_td = false, want_easter = false;
    for (int i = 2; i < argc; ++i) {
        std::string s = argv[i];
        if (s == "--amdid") do_amdid = true;
        if (s == "--automd") do_automd = true;
        if (s == "--trnaic") do_trnaic = true;
        if (s == "--aictest") do_aictest = true;
        if (s == "--td") want_td = true;
        if (s == "--easter") want_easter = true;
    }

    // --trnaic: report the automatic transform-selection result. run_m2 already
    // ran trnaic during the pre-model phase (transform{function=auto} sets
    // Fcntyp==0); print the two default-airline-model AICC values and the choice,
    // matching the oracle .udg aictest.trans.aicc.nolog / .log / aictrans keys.
    if (do_trnaic) {
        std::printf("OUTCOME: OK\n");
        if (!ctx.trnaic_result.ran) {
            std::printf("trnaic.ran: no\n");
            return 0;
        }
        std::printf("aictest.trans.aicc.nolog: %.15E\n",
                    ctx.trnaic_result.aicno);
        std::printf("aictest.trans.aicc.log: %.15E\n", ctx.trnaic_result.aiclog);
        std::printf("aictrans: %s\n",
                    ctx.trnaic_result.selected_log ? "Log(y)" : "None");
        return 0;
    }

    // --aictest: reproduce the automd preamble (default airline model built by
    // mdlint/mdlset/regvar) and drive the trading-day (tdaic) and/or Easter
    // (easaic) AIC tests on it, printing the AICC differences the oracle reports
    // as aictest.diff.td / aictest.diff.e. The aictest argument parser
    // (getreg/editor.f) that populates Tdayvc/Ntdvec/Easvec/... is not yet
    // ported, so the state that argument would set is installed here from the
    // spec's aictest=(td easter) defaults (no regime dates, no stock TD, no
    // pre-existing TD/Easter regressors). --td / --easter select the tests.
    if (do_aictest) {
        using namespace x13::prm;
        x13::model_cmn& m = ctx.model;
        x13::arima_cmn& ar = ctx.arima;

        bool afterauto = false;
        for (int i = 2; i < argc; ++i)
            if (std::string(argv[i]) == "--afterauto") afterauto = true;

        constexpr int PAt = x13::prm::PLEN + 2 * x13::prm::PORDER;
        std::vector<double> at(static_cast<std::size_t>(PAt), 0.0);
        // The transformed series (a SEPARATE buffer from ctx.series.tsrs, which
        // estimation overwrites with residuals). Saved now, before any estimation.
        std::vector<double> trnsaved(static_cast<std::size_t>(x13::prm::PLEN));
        x13::copy(ctx.series.tsrs.data(), x13::prm::PLEN, 1, trnsaved.data());
        std::vector<double> trn(static_cast<std::size_t>(x13::prm::PLEN));
        x13::copy(trnsaved.data(), x13::prm::PLEN, 1, trn.data());
        int frstry = 0, nefobs = 0, na = 0;

        if (afterauto) {
            // --afterauto: identify + estimate the model via the reduced automd
            // driver FIRST, then run the AIC tests on the IDENTIFIED model. This
            // reaches the oracle's SECOND aictest round (automd.f:514) for series
            // whose identified model differs from the default airline. The reduced
            // automd omits the aictest regressors during identification; where the
            // identified orders are unaffected, the round-2 result still matches.
            int fr2 = 0, nef2 = 0, nad2 = 0;
            x13::automd(ctx, trn.data(), fr2, nef2, at.data(), nad2);
            if (ctx.error.lfatal) { std::printf("OUTCOME: FATAL (automd)\n"); return 1; }
            // Restore the transformed series for the AIC tests (automd left
            // residuals in trn / ctx.series.tsrs).
            x13::copy(trnsaved.data(), x13::prm::PLEN, 1, trn.data());
        } else {
            // ---- default airline model (automd.f:193-212), no seasonal factor
            // for Sp==1 or seasonal-effect regressors. ----
            int lds0 = 1, lqs0 = 1;
            if (m.lseff || m.sp == 1) { lds0 = 0; lqs0 = 0; }
            bool inptok = true;
            x13::mdlint(ctx);
            x13::mdlset(ctx, 0, 1, 1, 0, lds0, lqs0, inptok);
            if (!ctx.error.lfatal)
                x13::regvar(ctx, trn.data(), ctx.extend.nobspf, ar.fctdrp,
                            ctx.extend.nfcst, 0, ar.userx.data(), ar.bgusrx.data(),
                            ar.nrusrx, ctx.prior.priadj, ar.reglom, ar.nrxy,
                            ar.begxy.data(), frstry, true, ar.elong);
            if (ctx.error.lfatal) { std::printf("OUTCOME: FATAL (preamble)\n"); return 1; }
        }

        // ---- prior-adjustment span (adjsrs.f:20-21,89-90). The ported pre-model
        // does the prior adjustment inline (run_pre_model.cpp) and never sets
        // Begadj/Nadj/Adj1st; tdaic's log-transform leap-year PREADJUSTMENT needs
        // them so td7var can build the length-of-month/leap-year factor. In real
        // X-13 adjsrs runs in editor before automd. Nbcst==0 for these specs. ----
        {
            int nbcst = ctx.extend.nbcst < 0 ? 0 : ctx.extend.nbcst;
            x13::addate(ctx.mdldat.begspn.data(), m.sp, -nbcst,
                        ctx.adj.begadj.data());
            int nfc = ctx.extend.nfcst < 0 ? 0 : ctx.extend.nfcst;
            int tail = m.sp > (nfc - ar.fctdrp) ? m.sp : (nfc - ar.fctdrp);
            ctx.adj.nadj = ctx.mdldat.nspobs + nbcst + tail;
            int a1st = 0;
            x13::dfdate(ctx.mdldat.begspn.data(), ctx.adj.begadj.data(), m.sp, a1st);
            ctx.adj.adj1st = a1st + 1;
        }

        // ---- aictest state the (unported) getreg/editor parser would set. ----
        ar.pvaic = DNOTST;                      // gtinpt.f:301
        for (int k = 1; k <= PAICT; ++k) ar.rgaicd(k) = 0.0;  // gtinpt.f:294
        ar.traicd = DNOTST;
        ar.lomtst = 0;
        ctx.picktd.lrgmtd = false;
        ctx.picktd.tdzero = 0;
        ctx.picktd.tddate(1) = NOTSET;
        ctx.picktd.tddate(2) = NOTSET;
        ar.aicstk = 0;
        m.easidx = 0;

        bool lester = false;
        bool preeas = false;
        for (int i = 2; i < argc; ++i)
            if (std::string(argv[i]) == "--preeas") preeas = true;

        // --preeas: install the Easter regressor (as automd's round-1 easaic
        // would) BEFORE tdaic, so tdaic estimates each TD candidate with Easter
        // present -- matching the oracle's round-2 model state, in which Easter
        // was already selected before the trading-day test re-runs (automd.f:514
        // tdaic sees the Easter group added by the round-1 easaic at line 230).
        // Only the reported diff.td is meaningful in this mode (the trailing
        // easaic's diff.e is reported by the plain --afterauto run instead).
        if (preeas && want_td && want_easter) {
            ar.eastst = 1; ar.neasvc = 4;
            ar.easvec(1) = -1; ar.easvec(2) = 1; ar.easvec(3) = 8; ar.easvec(4) = 15;
            ctx.x11adj.neas = 0;
            x13::easaic(ctx, trn.data(), at.data(), nefobs, na, frstry, lester,
                        /*lsumm=*/false);
            if (ctx.error.lfatal) { std::printf("OUTCOME: FATAL (pre-easaic)\n"); return 1; }
        }

        if (want_td) {
            // editor.f:1151-1166 for aictest=td (Itdtst=1), monthly/quarterly,
            // no existing TD regressor: Tdayvc = (0, 1, 4), Ntdvec = 3.
            ar.itdtst = 1;
            ar.ntdvec = 3;
            ar.tdayvc(1) = 0;
            ar.tdayvc(2) = 1;
            ar.tdayvc(3) = 4;
            int tdmdl1 = 0;
            x13::tdaic(ctx, trn.data(), at.data(), nefobs, na, frstry, tdmdl1,
                       /*ltdlom=*/false, lester, /*lsumm=*/false);
            if (ctx.error.lfatal) { std::printf("OUTCOME: FATAL (tdaic)\n"); return 1; }
        }

        if (want_easter) {
            // editor.f:1410-1442 for aictest=easter, no existing Easter regressor:
            // Easvec = (-1, 1, 8, 15), Neasvc = 4, Eastst = 1, Neas = 0.
            ar.eastst = 1;
            ar.neasvc = 4;
            ar.easvec(1) = -1;
            ar.easvec(2) = 1;
            ar.easvec(3) = 8;
            ar.easvec(4) = 15;
            ctx.x11adj.neas = 0;
            x13::easaic(ctx, trn.data(), at.data(), nefobs, na, frstry, lester,
                        /*lsumm=*/false);
            if (ctx.error.lfatal) { std::printf("OUTCOME: FATAL (easaic)\n"); return 1; }
        }

        std::printf("OUTCOME: OK\n");
        if (want_td) {
            std::printf("aictest.td: %s\n", ar.aicint > 0 ? "yes" : "no");
            std::printf("aictest.aicint: %d\n", ar.aicint);
            std::printf("aictest.diff.td: %.10E\n", ar.dfaict);
        }
        if (want_easter) {
            std::printf("aictest.e: %s\n", ar.aicind >= 0 ? "yes" : "no");
            std::printf("aictest.e.window: %d\n", ar.aicind);
            std::printf("aictest.diff.e: %.10E\n", ar.dfaice);
        }
        std::printf("estim.error: %s\n", lester ? "yes" : "no");
        return 0;
    }

    // --automd: run the unified (reduced) driver end to end, then read back the
    // identified orders and estimation results.
    if (do_automd) {
        constexpr int PAd = x13::prm::PLEN + 2 * x13::prm::PORDER;
        std::vector<double> ad(static_cast<std::size_t>(PAd), 0.0);
        std::vector<double> trn(static_cast<std::size_t>(x13::prm::PLEN));
        x13::copy(ctx.series.tsrs.data(), x13::prm::PLEN, 1, trn.data());
        int fr = 0, nef = 0, nad = 0;
        try {
            x13::automd(ctx, trn.data(), fr, nef, ad.data(), nad);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "x13run_iddiff: automd exception: %s\n", e.what());
            return 3;
        }
        if (ctx.error.lfatal) {
            std::printf("OUTCOME: FATAL (automd)\n");
            return 1;
        }
        int ipr, ips, idr2, ids2, iqr, iqs, id, ip, iq, iprs, iqrs, n;
        x13::cnvmdl(ctx, ipr, ips, idr2, ids2, iqr, iqs, id, ip, iq, iprs, iqrs, n);
        std::printf("OUTCOME: OK\n");
        if (ips == 0 && ids2 == 0 && iqs == 0)
            std::printf("arimamdl: (%d %d %d)\n", ipr, idr2, iqr);
        else
            std::printf("arimamdl: (%d %d %d)(%d %d %d)\n", ipr, idr2, iqr, ips,
                        ids2, iqs);
        std::printf("variance: %.10E\n", ctx.mdldat.var);
        std::printf("loglikelihood: %.10E\n", ctx.mdldat.lnlkhd);
        std::printf("checkmu: %s\n", ctx.model.nb > 0 ? "yes" : "no");
        return 0;
    }

    constexpr int PA = x13::prm::PLEN + 2 * x13::prm::PORDER;
    std::vector<double> a(static_cast<std::size_t>(PA), 0.0);
    int idr = maxdr, ids = maxds, nefobs = 0, frstry = 0, na = 0;
    bool lmu = false;

    // The identification routines take Trnsrs -- the transformed series -- as a
    // read-only input, a SEPARATE buffer from Tsrs (which rgarma overwrites with
    // regression residuals during estimation). Give them their own copy so an
    // internal rgarma cannot clobber the series a later stage reads.
    std::vector<double> trnsrs(static_cast<std::size_t>(x13::prm::PLEN));
    x13::copy(ctx.series.tsrs.data(), x13::prm::PLEN, 1, trnsrs.data());

    try {
        x13::iddiff(ctx, idr, ids, trnsrs.data(), nefobs, frstry, a.data(),
                    na, /*imu=*/0, lmu, /*svldif=*/false, /*lsumm=*/0);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "x13run_iddiff: iddiff exception: %s\n", e.what());
        return 3;
    }
    if (ctx.error.lfatal) {
        std::printf("OUTCOME: FATAL (iddiff)\n");
        return 1;
    }

    std::printf("OUTCOME: OK\n");
    std::printf("idnonseasonaldiff.first: %d\n", idr);
    std::printf("idseasonaldiff.first: %d\n", ids);
    std::printf("checkmu: %s\n", lmu ? "yes" : "no");

    // Optional: continue into ARMA-order identification (amdid) on the chosen
    // differencing. Requires an automdl{} spec (maxorder/maxdiff defaults).
    if (do_amdid) {
        int irar = 0, irma = 0, isar = 0, isma = 0;
        bool locok = true;
        try {
            x13::amdid(ctx, irar, idr, irma, isar, ids, isma, trnsrs.data(),
                       frstry, nefobs, a.data(), na, lmu, /*lsumm=*/0, locok);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "x13run_iddiff: amdid exception: %s\n", e.what());
            return 3;
        }
        if (ctx.error.lfatal) {
            std::printf("OUTCOME: FATAL (amdid)\n");
            return 1;
        }
        // Match the oracle .udg format: omit the seasonal group when it is all
        // zero (the (P D Q) block is only printed when present).
        if (isar == 0 && ids == 0 && isma == 0)
            std::printf("arimamdl: (%d %d %d)\n", irar, idr, irma);
        else
            std::printf("arimamdl: (%d %d %d)(%d %d %d)\n", irar, idr, irma, isar,
                        ids, isma);
    }
    return 0;
}
