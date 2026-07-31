// x13run_m3.cpp -- M3 CLI test harness: parse a spec, run the pre-model phase
// AND estimate the regARIMA model (x13::run_m2 with estimate=true), then print
// the estimation results as `KEY value` lines to stdout for the estimation
// parity gate to diff against the oracle .udg goldens.
//
// Usage: x13run_m3 <specfile.spc>
//
// This harness deliberately does NOT write any output file -- the library
// captures everything on the context (the package's "no auto file output"
// rule); only this thin test driver surfaces it, and only to stdout. The full
// oracle-format .udg emission is a separate (deferred) milestone; here we print
// the numeric estimation summary the gate needs.
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

#include <cmath>
#include <vector>
#include "x13/fformat.hpp"
#include "specparse/specparse.hpp"
#include "gen/model.hpp"  // prm::AR, prm::MA

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

// arima.f / acfdgn.f / nrmtst.f savelog block -- the check{} residual
// diagnostics, written through the port's own Fortran-format facility with the
// EXACT format each line is emitted with in the oracle, so the gate can compare
// against the golden `.udg` text directly:
//   acfdgn 1110: (a,': ',f7.4)                            qlimit / acflimit
//   acfdgn 1160: ('n',a,'q: ',i3)                         nlbq / nbpq
//   acfdgn 1150: (a,'q$',i2.2,': ',f7.3,5x,i3,5x,f6.3)    lbq$NN / bpq$NN
//   acfdgn 1170: ('nsig',a,': ',i3)                       nsigacf / nsigpacf
//   acfdgn 1180: ('sig',a,'$',i2.2,': ',f7.4,5x,f7.4,3x,f7.4)
//   acfdgn 1190: (a,'lags: ',a)                           lblags / bplags / sig*lags
//   nrmtst 1030: (a,':',f10.4,1x,a)                       skewness / a / kurtosis
//   arima  9000: (a,e15.8)                                durbinwatson
//   arima  9001: (a,e15.8,1x,i3,1x,e15.8)                 friedman
//
// Going through fwrite_fmt rather than printf matters: a value too wide for its
// field is filled with '*' in Fortran, and the corpus reaches that -- a partial
// autocorrelation with |t| = 10.169 does not fit acfdgn's f7.4, so the oracle
// prints `*******` (unrate_sar-seats, sigpacf$24).
static void dump_check(const x13::X13Context& ctx) {
    using x13::fwrite_fmt;
    const auto& ck = ctx.check;
    if (!ck.ran) return;

    auto lags = [](const std::vector<x13::CheckLag>& v) {
        if (v.empty()) return std::string("0");
        std::string s;
        for (const auto& L : v) s += std::to_string(L.lag) + " ";
        return s;
    };
    auto line = [](const std::string& t) { std::printf("%s\n", t.c_str()); };

    line(fwrite_fmt("(a,': ',f7.4)", "qlimit", ck.qlimit));
    line(fwrite_fmt("('n',a,'q: ',i3)", "lb", static_cast<int>(ck.lbq.size())));
    for (const auto& L : ck.lbq)
        line(fwrite_fmt("(a,'q$',i2.2,': ',f7.3,5x,i3,5x,f6.3)", "lb", L.lag,
                        L.a, L.df, L.b));
    line(fwrite_fmt("(a,'lags: ',a)", "lb", lags(ck.lbq)));

    line(fwrite_fmt("('n',a,'q: ',i3)", "bp", static_cast<int>(ck.bpq.size())));
    for (const auto& L : ck.bpq)
        line(fwrite_fmt("(a,'q$',i2.2,': ',f7.3,5x,i3,5x,f6.3)", "bp", L.lag,
                        L.a, L.df, L.b));
    line(fwrite_fmt("(a,'lags: ',a)", "bp", lags(ck.bpq)));

    line(fwrite_fmt("(a,': ',f7.4)", "acflimit", ck.acflimit));
    line(fwrite_fmt("('nsig',a,': ',i3)", "acf",
                    static_cast<int>(ck.sigacf.size())));
    for (const auto& L : ck.sigacf)
        line(fwrite_fmt("('sig',a,'$',i2.2,': ',f7.4,5x,f7.4,3x,f7.4)", "acf",
                        L.lag, L.a, L.b, L.t));
    line(fwrite_fmt("(a,'lags: ',a)", "sigacf", lags(ck.sigacf)));

    line(fwrite_fmt("('nsig',a,': ',i3)", "pacf",
                    static_cast<int>(ck.sigpacf.size())));
    for (const auto& L : ck.sigpacf)
        line(fwrite_fmt("('sig',a,'$',i2.2,': ',f7.4,5x,f7.4,3x,f7.4)", "pacf",
                        L.lag, L.a, L.b, L.t));
    line(fwrite_fmt("(a,'lags: ',a)", "sigpacf", lags(ck.sigpacf)));

    if (ck.have_skew)
        line(fwrite_fmt("(a,':',f10.4,1x,a)", "skewness", ck.skewness,
                        std::string(1, ck.skew_mark)));
    if (ck.have_geary)
        line(fwrite_fmt("(a,':',f10.4,1x,a)", "a", ck.geary,
                        std::string(1, ck.geary_mark)));
    if (ck.have_kurt)
        line(fwrite_fmt("(a,':',f10.4,1x,a)", "kurtosis", ck.kurtosis,
                        std::string(1, ck.kurt_mark)));
    if (ck.have_dw)
        line(fwrite_fmt("(a,e15.8)", "durbinwatson: ", ck.dw));
    if (ck.have_friedman)
        line(fwrite_fmt("(a,e15.8,1x,i3,1x,e15.8)", "friedman: ", ck.friedman,
                        ck.friedman_df, ck.friedman_pv));
}

// savotl.f / prtrts.f savelog block -- the outlier counts and the ARMA operator
// roots, in the oracle's own formats:
//   savotl 1080: (a,i2)                        outlier.ao / .ls / ... / .total
//   prtrts 1031: (a,a,a,a,i2.2,a,a)            roots.<filter>.<period>.<NN>
// The four root values come from `dtoc`, which writes E22.15 with a forced
// sign, TAB-separated.
static void dump_estdgn(const x13::X13Context& ctx) {
    using x13::fwrite_fmt;
    const auto& ed = ctx.estdgn;
    if (!ed.ran) return;
    auto line = [](const std::string& t) { std::printf("%s\n", t.c_str()); };

    line(fwrite_fmt("(a,i2)", "outlier.ao: ", ed.ao));
    line(fwrite_fmt("(a,i2)", "outlier.ls: ", ed.ls));
    line(fwrite_fmt("(a,i2)", "outlier.tc: ", ed.tc));
    line(fwrite_fmt("(a,i2)", "outlier.so: ", ed.so));
    line(fwrite_fmt("(a,i2)", "outlier.rp: ", ed.rp));
    line(fwrite_fmt("(a,i2)", "outlier.tls: ", ed.tls));
    if (ed.have_user)
        line(fwrite_fmt("(a,i2)", "outlier.user: ", ed.user));
    line(fwrite_fmt("(a,i2)", "outlier.total: ", ed.total));
    if (ed.have_autoout)
        line(fwrite_fmt("(a,i2)", "autoout: ", ed.autoout));

    for (const auto& r : ed.roots) {
        const std::string key =
            fwrite_fmt("(a,a,a,a,i2.2,a)", "roots." + r.filter, ".", r.period,
                       ".", r.index, ": ");
        line(key + fwrite_fmt("(sp,e22.15)", r.real) + "\t" +
             fwrite_fmt("(sp,e22.15)", r.imag) + "\t" +
             fwrite_fmt("(sp,e22.15)", r.modulus) + "\t" +
             fwrite_fmt("(sp,e22.15)", r.frequency));
    }
}


// prtmdl.f savelog block -- the model shape counters and the ARMA coefficient
// table:
//   prtmdl 1000: (a,i3)                              nonseasonaldiff / ... / nmodel
//   prtmdl 1261: (a,a,a,a,i2.2,a,i2.2,a,sp,3(e21.14,a),a)
// The coefficient key keeps the operator title's own CASE (MA$Nonseasonal$01$01),
// unlike the roots key beside it, which lowercases both halves.
static void dump_estmdl(const x13::X13Context& ctx) {
    using x13::fwrite_fmt;
    const auto& ed = ctx.estdgn;
    if (!ed.ran) return;
    auto line = [](const std::string& t) { std::printf("%s\n", t.c_str()); };

    line(fwrite_fmt("(a,i3)", "nonseasonaldiff: ", ed.nonseasonaldiff));
    line(fwrite_fmt("(a,i3)", "seasonaldiff: ", ed.seasonaldiff));
    line(fwrite_fmt("(a,i3)", "nmodel: ", ed.nmodel));

    for (const auto& c : ed.coefs) {
        const std::string cfix = c.fixed ? "(fixed)" : "       ";
        line(fwrite_fmt("(a,a,a,a,i2.2,a,i2.2,a,sp,3(e21.14,a),a)",
                        c.filter, "$", c.period, "$", c.factor, "$", c.lag,
                        ": ", c.value, " ", c.se, " ", c.t, " ", cfix));
    }
}


// arima.f:888-898 -- the average absolute percentage forecast error:
//   arima 1200: (a,f12.4)     aape.0 / .1 / .2 / .3
// with a plain `aape.mode:` line naming the variant. Emit order is the
// oracle's: the THREE-YEAR AVERAGE (mape(4)) is written first, as `aape.0`.
static void dump_aape(const x13::X13Context& ctx) {
    using x13::fwrite_fmt;
    const auto& a = ctx.aape;
    auto line = [](const std::string& t) { std::printf("%s\n", t.c_str()); };
    if (!a.ok) {
        line("aape.mode: none");
        return;
    }
    line(a.outofsample ? "aape.mode: outofsample" : "aape.mode: withinsample");
    line(fwrite_fmt("(a,f12.4)", "aape.0: ", a.mape[3]));
    line(fwrite_fmt("(a,f12.4)", "aape.1: ", a.mape[0]));
    line(fwrite_fmt("(a,f12.4)", "aape.2: ", a.mape[1]));
    line(fwrite_fmt("(a,f12.4)", "aape.3: ", a.mape[2]));

    // The BACKCAST twin (automx.f:906). The oracle writes no savelog key for it
    // -- prtamd PRINTS it -- so this is harness surface, and the gate compares
    // it against the printed "Average absolute percentage error in ... backcasts"
    // block of the .out. Emitted at prtamd's own two-decimal precision.
    if (ctx.aape_bcst_ran && ctx.aape_bcst.ok) {
        const auto& b = ctx.aape_bcst;
        line(b.outofsample ? "bcstaape.mode: outofsample"
                           : "bcstaape.mode: withinsample");
        line(fwrite_fmt("(a,f12.4)", "bcstaape.0: ", b.mape[3]));
        line(fwrite_fmt("(a,f12.4)", "bcstaape.1: ", b.mape[0]));
        line(fwrite_fmt("(a,f12.4)", "bcstaape.2: ", b.mape[1]));
        line(fwrite_fmt("(a,f12.4)", "bcstaape.3: ", b.mape[2]));
    }
}


// svaict.f savelog block -- the `aictest.*` keys. The oracle's own FORMATs, so
// the gate compares the golden `.udg` text directly:
//   1010: (a:,a)          the label / yes / no / nomodel lines
//   1020: (a,i6)          aictest.e.window, single value
//   1025: (a,5i6)         aictest.e.window, the Aicind==99 multi-window form
//   1040: (a,': ',e20.10) aictest.diff.* and aictest.cvaic.*
// Note 1040 supplies its own ': ' where 1010/1020/1025 carry it in the string.
static void dump_aictest(const x13::X13Context& ctx) {
    using x13::fwrite_fmt;
    const auto& s = ctx.aictest_log;
    if (!s.ran) return;
    auto line = [](const std::string& t) { std::printf("%s\n", t.c_str()); };

    auto verdict = [&](const x13::AictestSavelog::Group& g, const std::string& key,
                       bool label_when_yes) {
        if (g.nomodel) {
            line(fwrite_fmt("(a:,a)", key + ": ", std::string("nomodel")));
            return;
        }
        if (!g.accepted)
            line(fwrite_fmt("(a:,a)", key + ": ", std::string("no")));
        else if (label_when_yes)
            line(fwrite_fmt("(a:,a)", key + ": ", g.label));
        else
            line(fwrite_fmt("(a:,a)", key + ": ", std::string("yes")));
    };
    auto diffs = [&](const x13::AictestSavelog::Group& g, const std::string& stem) {
        if (g.nomodel) return;
        line(fwrite_fmt("(a,': ',e20.10)", "aictest.diff." + stem, g.diff));
        if (g.have_cvaic)
            line(fwrite_fmt("(a,': ',e20.10)", "aictest.cvaic." + stem, g.cvaic));
    };

    if (s.td.tested) {
        // The trading-day key carries the LABEL when accepted, where every
        // other group carries a bare `yes` (svaict.f:38 vs :71).
        verdict(s.td, "aictest.td", /*label_when_yes=*/true);
        diffs(s.td, "td");
    }
    if (s.lom.tested) {
        line(fwrite_fmt("(a:,a)", "aictest." + s.lom_abbrev + ".reg: ", s.lom.label));
        verdict(s.lom, "aictest." + s.lom_abbrev, false);
        diffs(s.lom, s.lom_abbrev);
    }
    if (s.easter.tested) {
        line(fwrite_fmt("(a:,a)", "aictest.easter.reg: ", s.easter.label));
        verdict(s.easter, "aictest.e", false);
        if (s.easter_window.size() > 1) {
            std::string t = "aictest.e.window: ";
            for (int w : s.easter_window) t += fwrite_fmt("(i6)", w);
            line(t);
        } else if (!s.easter_window.empty()) {
            line(fwrite_fmt("(a,i6)", "aictest.e.window: ", s.easter_window[0]));
        }
        diffs(s.easter, "e");
    }
    if (s.user.tested) {
        verdict(s.user, "aictest.u", false);
        diffs(s.user, "u");
    }
}


int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: x13run_m3 <specfile.spc>\n");
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
        std::fprintf(stderr, "x13run_m3: cannot open %s\n", path.c_str());
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
        ok = x13::run_m2(ctx, text, base, /*estimate=*/true);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "x13run_m3: exception: %s\n", e.what());
        return 3;
    }

    std::printf("OUTCOME: %s\n", ok ? "OK" : "FATAL");
    if (!ok) return 1;
    if (!ctx.captured.has_model) {
        std::printf("MODEL: none\n");  // no arima model -> nothing to estimate
        return 0;
    }

    const auto& m = ctx.model;
    const auto& d = ctx.mdldat;
    const auto& lk = ctx.lkhd;
    // Nspobs is the SERIES span after a modelspan run (setspn.f restores it), so
    // prefer the count the fit recorded; fall back for the no-estimate paths.
    int nefobs = ctx.est_nefobs > 0 ? ctx.est_nefobs : d.nspobs - m.nintvl;

    // Integer counters + dimensions.
    std::printf("converged: %s\n", d.convrg ? "yes" : "no");
    std::printf("armaer: %d\n", d.armaer);
    std::printf("niter: %d\n", d.nliter);
    std::printf("nfev: %d\n", d.nfev);
    std::printf("nreg: %d\n", m.nb);
    std::printf("nefobs: %d\n", nefobs);

    // Scalar estimation results (%.14E mirrors the oracle Nform precision).
    std::printf("variance: %.14E\n", d.var);
    std::printf("loglikelihood: %.14E\n", d.lnlkhd);
    std::printf("aic: %.14E\n", lk.aic);
    std::printf("aicc: %.14E\n", lk.aicc);
    std::printf("bic: %.14E\n", lk.bic);
    std::printf("hq: %.14E\n", lk.hnquin);
    dump_check(ctx);
    dump_estdgn(ctx);
    dump_estmdl(ctx);
    dump_aape(ctx);
    dump_aictest(ctx);

    // ARMA coefficients in operator/lag order (AR then MA), skipping the fixed
    // differencing slots; label each free coef by type + lag.
    for (int iflt = x13::prm::AR; iflt <= x13::prm::MA; ++iflt) {
        int begopr = m.mdl(iflt - 1);
        int endopr = m.mdl(iflt) - 1;
        const char* kind = (iflt == x13::prm::AR) ? "ar" : "ma";
        for (int iopr = begopr; iopr <= endopr; ++iopr) {
            int beglag = m.opr(iopr - 1);
            int endlag = m.opr(iopr) - 1;
            for (int ilag = beglag; ilag <= endlag; ++ilag)
                std::printf("arima.%s.lag%d: %.14E\n", kind, m.arimal(ilag),
                            d.arimap(ilag));
        }
    }

    // Regression betas in design-column order.
    for (int j = 1; j <= m.nb; ++j)
        std::printf("beta%d: %.14E\n", j, d.b(j));

    // Forecast-output table (fcstout / prtfct .fct path): original-scale point
    // forecast + two-tailed confidence band, one line per lead.
    const auto& fo = ctx.forecasts;
    std::printf("nfcst: %d\n", fo.nfcst);
    for (int i = 0; i < fo.nfcst; ++i)
        std::printf("fcst%d: %.14E %.14E %.14E\n", i + 1, fo.fcst[i],
                    fo.lwrci[i], fo.uprci[i]);

    return 0;
}
