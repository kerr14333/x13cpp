// x13run_seats.cpp -- SEATS CLI harness: parse a spec, estimate the regARIMA
// model, then dispatch to x13::run_seats (the SEATS decomposition entry
// point). Modeled directly on tools/x13run_x11.cpp.
//
// Usage: x13run_seats <specfile.spc>
//
// STATUS (see tools/seats_scope.md): run_seats() now succeeds (OUTCOME: OK)
// for the additive/no-seasonal/no-cycle historical-span case (unrate_seats,
// session 12 -- ESTBUR's general MLTSOL branch, core/src/seats/estbur.cpp).
// Every other corpus spec still fatals cleanly at the decomposition dispatch
// point. This harness re-runs the same (idempotent) decode -> canonical-
// denoms -> SPECTRU -> DecompSpectrum -> ESTBUR chain run_seats() uses
// internally (x13context.hpp has nowhere to stash the result -- out of scope
// for this driver, owned by the concurrent span agent) purely to dump the
// s-tables in the `<tag> YYYYMM <value>` format tests/parity/
// test_seats_tables.py expects.
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

#include "specparse/specparse.hpp"
#include "common/x13context.hpp"
#include "seats/model_decode.hpp"
#include "seats/canonical_denoms.hpp"
#include "seats/spectru.hpp"
#include "seats/seatopts.hpp"
#include "seats/decompspectrum.hpp"
#include "seats/estbur.hpp"

namespace {
std::string dirname_of(const std::string& p) {
    std::size_t s = p.find_last_of("/\\");
    return (s == std::string::npos) ? std::string() : p.substr(0, s);
}
std::string basename_of(const std::string& p) {
    std::size_t s = p.find_last_of("/\\");
    return (s == std::string::npos) ? p : p.substr(s + 1);
}

// Emit one table section: `<tag> YYYYMM <value>` over [1, n] (matching
// tests/parity/test_seats_tables.py's parser -- same shape as
// tools/x13run_x11.cpp's own dump()).
void dump(const char* tag, const int* begspn, int sp, int n,
          const double* arr /*0-based*/) {
    for (int i = 1; i <= n; ++i) {
        int idate[2];
        x13::addate(begspn, sp, i - 1, idate);
        std::printf("%s %04d%02d %.15E\n", tag, idate[0], idate[1],
                    arr[i - 1]);
    }
}
}  // namespace

int main(int argc, char** argv) {
    std::setvbuf(stderr, nullptr, _IONBF, 0);
    if (argc < 2) {
        std::fprintf(stderr, "usage: x13run_seats <specfile.spc>\n");
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
        std::fprintf(stderr, "x13run_seats: cannot open %s\n", path.c_str());
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
        ok = x13::run_seats(ctx, text, base);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "x13run_seats: exception: %s\n", e.what());
        return 3;
    }

    std::printf("OUTCOME: %s\n", ok ? "OK" : "FATAL");

    // Model-decode + canonical-denominator + SPECTRU + DecompSpectrum +
    // ESTBUR probe (tools/seats_scope.md next-increment #1/#2/#3, sessions
    // 3-4, 6, 12): ctx.model/ctx.mdldat already hold the fitted regARIMA
    // model at this point (run_m2_after_parse ran to completion), so this
    // harness exercises the whole chain on the REAL fitted model and prints
    // diagnostics for the pytest gates to diff against golden .udg/.mdc/s-
    // table values. Best-effort: swallow any exception so this extra probe
    // never turns a clean "OUTCOME: FATAL" run into a crash.
    x13::SeatsCanonicalDenoms cd;
    x13::SeatsComponentModels comp;
    x13::EstburResult est;
    bool have_est = false;
    try {
        x13::SeatsOptions opts = x13::seats_resolve_options(ctx);
        x13::SeatsModelOrders mo;
        if (x13::seats_decode_model(ctx, opts.xl, mo)) {
            std::printf("DECODE_ARIMAMDL: (%d %d %d)", mo.p, mo.d, mo.q);
            if (mo.bp > 0 || mo.bd > 0 || mo.bq > 0)
                std::printf("(%d %d %d)", mo.bp, mo.bd, mo.bq);
            std::printf("\n");

            x13::seats_canonical_denoms(mo, opts.rmod, opts.epsphi, cd);

            x13::SpectruResult sr;
            x13::spectru(cd.thstar, cd.qstar, cd.chi, cd.nchi, cd.cyc,
                         cd.ncyc, cd.psi, cd.npsi, cd.pstar, mo.mq, mo.bd,
                         mo.d, /*out=*/1, /*har=*/0, cd.root0c, cd.rootpic,
                         cd.rootpis, sr);
            std::printf("DECODE_QT1: %.15g\n", sr.qt1);

            // DecompSpectrum/MAspectrum probe (tools/seats_scope.md
            // next-increment, session 6): produces THETP/THETS/THETC/THADJ
            // + variances, the source of the .mdc sanum/saden/savar/trnum/
            // trden/trvar keys (traced via ansub9.f's USRENTRY IFUNC=2001-
            // 2013 dispatch, session 5). Printed in the SAME "key: value"
            // shape the golden .mdc file uses (MDC_ prefixed) so the pytest
            // gate can parse both with one regex.
            x13::decomp_spectrum(sr, cd, cd.is_close_to_td, comp);

            // snum/sden/svar <- THETS/PSI/VARWNS (ShowComp's "npsi!=1"
            // guard, spectrum.f:2593-2607 -- only emitted when the model has
            // real seasonal structure; airline_seats/*-fixed-airline-seats
            // are the only corpus specs that do).
            if (cd.npsi != 1) {
                if (comp.nthets > 0) {
                    std::printf("MDC_nsnum: %d\n", comp.nthets);
                    for (int i = 0; i < comp.nthets; ++i)
                        std::printf("MDC_snum.%03d: %.15g\n", i,
                                     comp.thets[i]);
                }
                if (cd.npsi > 0) {
                    std::printf("MDC_nsden: %d\n", cd.npsi);
                    for (int i = 0; i < cd.npsi; ++i)
                        std::printf("MDC_sden.%03d: %.15g\n", i, cd.psi[i]);
                }
                std::printf("MDC_svar: %.15g\n", comp.varwns);
            }

            // sanum/saden/savar <- THADJ/CHCYC/VARWNA (spectrum.f:2817-2884,
            // always computed when nchcyc!=1 or ncycth!=0 -- true whenever
            // there is any differencing at all).
            if (comp.nthadj > 0) {
                std::printf("MDC_nsanum: %d\n", comp.nthadj);
                for (int i = 0; i < comp.nthadj; ++i)
                    std::printf("MDC_sanum.%03d: %.15g\n", i, comp.thadj[i]);
            }
            if (cd.nchcyc > 0) {
                std::printf("MDC_nsaden: %d\n", cd.nchcyc);
                for (int i = 0; i < cd.nchcyc; ++i)
                    std::printf("MDC_saden.%03d: %.15g\n", i, cd.chcyc[i]);
            }
            std::printf("MDC_savar: %.15g\n", comp.varwna);

            // trnum/trden/trvar <- THETC/CYC/VARWNC (spectrum.f:2626-2641's
            // ShowComp guard: only emitted when ncycth!=0 or ncyc!=1).
            if (sr.ncycth != 0 || cd.ncyc != 1) {
                if (comp.nthetc > 0) {
                    std::printf("MDC_ntrnum: %d\n", comp.nthetc);
                    for (int i = 0; i < comp.nthetc; ++i)
                        std::printf("MDC_trnum.%03d: %.15g\n", i,
                                     comp.thetc[i]);
                }
                if (cd.ncyc > 0) {
                    std::printf("MDC_ntrden: %d\n", cd.ncyc);
                    for (int i = 0; i < cd.ncyc; ++i)
                        std::printf("MDC_trden.%03d: %.15g\n", i,
                                     cd.cyc[i]);
                }
                std::printf("MDC_trvar: %.15g\n", comp.varwnc);
            }

            // ESTBUR historical-span solve (session 12) -- s11 (SA)/s12
            // (trend)/s13 (irregular)/s18. Only meaningful once run_seats()
            // itself reports OK (matching the driver's own gating); still
            // computed best-effort here for diagnostics even when it isn't.
            x13::estbur_historical(ctx, mo, cd, comp, opts, est);
            have_est = est.ok;
        }
    } catch (const std::exception&) {
        // Decode/SPECTRU/ESTBUR is best-effort diagnostic output, not part
        // of the driver's contract.
    }

    if (!ok) return 1;

    if (have_est) {
        const int* begspn = ctx.mdldat.begspn.data();
        int sp = ctx.model.sp;
        int n = static_cast<int>(est.trend.size());
        dump("s11", begspn, sp, n, est.sa.data());
        dump("s12", begspn, sp, n, est.trend.data());
        dump("s13", begspn, sp, n, est.ir.data());
        // s10 = seasonal-only factor (linearized/sa); s16/s18 = combined
        // adjustment factors (original/sa, calendar/outliers refolded). They
        // coincide for a no-regressor or Constant-only model (est.combined_*
        // collapses onto seasonal_*), and diverge once TD/holiday/outliers are
        // stripped from the decomposition input (estbur.hpp / run_seats wiring).
        dump("s10", begspn, sp, n, est.seasonal_add.data());
        dump("s16", begspn, sp, n, est.combined_add.data());
        dump("s18", begspn, sp, n, est.combined_factor.data());
        // s14 -- transitory component (SEATS). Non-trivial only for AR/cycle
        // models; airline-family est.cycle is all-1 (no transitory).
        dump("s14", begspn, sp, n, est.cycle.data());
    }
    return 0;
}
