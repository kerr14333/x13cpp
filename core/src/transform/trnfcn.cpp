// trnfcn.cpp -- trnfcn.f: Box-Cox / logit series transformation.
#include "transform/transform.hpp"

#include <cmath>
#include <string>

#include "specparse/specparse.hpp"   // abend, STDERR
#include "numeric/numeric.hpp"       // dpeq (dpeq.f tolerance equality)
#include "x13/fformat.hpp"

namespace x13 {

namespace {
constexpr int PSTOP = 10;
constexpr double ZO = 0.0, ONE = 1.0;

// Emit the two-channel (STDERR + Mt2) error record for an out-of-domain value.
// trnfcn.f writes the STDERR and Mt2 diagnostics separately; for the log-of-zero
// case the two channels use different wording (trnfcn.f:86-87). what_mt2 supplies
// the Mt2 text when it differs; otherwise both channels get `what`.
void trnerr(X13Context& ctx, const std::string& fmt, const char* what, int i,
            double val, const char* what_mt2 = nullptr) {
    std::string rec = fwrite_fmt(fmt, std::string(what), i, val);
    ctx.channels_.unit(stdio::STDERR).put(rec + "\n");
    std::string rec2 =
        what_mt2 ? fwrite_fmt(fmt, std::string(what_mt2), i, val) : rec;
    ctx.channels_.unit(ctx.units.mt2).put(rec2 + "\n");
}
}  // namespace

// trnfcn.f
void trnfcn(X13Context& ctx, const double* y, int nsrs, int fcntyp, double lam,
            double* trny) {
    bool lstop = false;
    int nstop = 0;

    // FORMAT strings (1010 logit, 1020 log/box-cox, 1030 max-errors).
    static const char FMT1010[] =
        "(/,' ERROR: Cannot ',a,' a proportion not in the range ',"
        "'(0,1), y(',i5,')=',1p,g16.8,'.',/)";
    static const char FMT1020[] =
        "(' ERROR: Do not take ',a,', y(',i5,')=',1p,g16.8,'.')";
    static const char FMT1030[] =
        "(' ERROR: Maximum number of errors printed.  More errors ',"
        "'may exist, but',/,"
        "'        will not be specified.  The above values ',"
        "'cannot be processed.')";

    auto maxerr = [&]() {
        std::string rec = fwrite_fmt(FMT1030);
        ctx.channels_.unit(stdio::STDERR).put(rec + "\n");
        ctx.channels_.unit(ctx.units.mt2).put(rec + "\n");
        abend(ctx);
    };

    if (fcntyp == 3) {
        // Logit: log(y/(1-y)) for y in (0,1).
        for (int i = 1; i <= nsrs; ++i) {
            double tmp = y[i - 1];
            if (tmp > ZO && tmp < ONE) {
                trny[i - 1] = std::log(tmp / (ONE - tmp));
            } else {
                trnerr(ctx, FMT1010, "take the logit of", i, tmp);
                lstop = true;
                if (++nstop > PSTOP) { maxerr(); return; }
            }
        }
    } else if (dpeq(lam, ONE)) {
        for (int i = 1; i <= nsrs; ++i) trny[i - 1] = y[i - 1];
    } else if (dpeq(lam, ZO)) {
        for (int i = 1; i <= nsrs; ++i) {
            double yi = y[i - 1];
            if (yi > ZO) {
                trny[i - 1] = std::log(yi);
            } else {
                // trnfcn.f:86-87: STDERR says "log of zero", Mt2 "log of a zero".
                trnerr(ctx, FMT1020,
                       yi < ZO ? "log of a negative number" : "log of zero", i, yi,
                       yi < ZO ? "log of a negative number" : "log of a zero");
                lstop = true;
                if (++nstop > PSTOP) { maxerr(); return; }
            }
        }
    } else {
        // Box-Cox: lam^2 + (y^lam - 1)/lam,  y>0.
        for (int i = 1; i <= nsrs; ++i) {
            double yi = y[i - 1];
            if (yi > ZO) {
                trny[i - 1] = lam * lam + (std::pow(yi, lam) - ONE) / lam;
            } else {
                trnerr(ctx, FMT1020, "BoxCox transform", i, yi);
                lstop = true;
                if (++nstop > PSTOP) { maxerr(); return; }
            }
        }
    }

    if (lstop) abend(ctx);
}

// invfcn.f -- inverse Box-Cox / logit transform (see hpp).
void invfcn(X13Context& ctx, const double* trny, int nsrs, int fcntyp,
            double lam, double* y) {
    constexpr double ZO = 0.0, ONE = 1.0;
    (void)ctx;  // only the deferred out-of-domain diagnostic uses it
    if (fcntyp == 3) {                        // inverse logit
        for (int i = 1; i <= nsrs; ++i) {
            double tmp = std::exp(trny[i - 1]);
            y[i - 1] = tmp / (ONE + tmp);
        }
    } else if (dpeq(lam, ONE) || fcntyp == 4) {  // identity
        copy(trny, nsrs, 1, y);
    } else if (dpeq(lam, ZO) || fcntyp == 1) {   // exp
        for (int i = 1; i <= nsrs; ++i) y[i - 1] = std::exp(trny[i - 1]);
    } else {                                  // inverse box-cox
        double invlam = ONE / lam;
        for (int i = 1; i <= nsrs; ++i) {
            double fact = lam * (trny[i - 1] - lam * lam) + ONE;
            if (fact > ZO) y[i - 1] = std::pow(fact, invlam);
            // else: leave y(i) unchanged; the Cox-Box diagnostic is deferred.
        }
    }
}

// lgnrmc.f -- lognormal mean-correction of forecasts (see hpp).
void lgnrmc(int nfcst, const double* fctunc, const double* fctse,
            double* fctcor, bool ltrans) {
    constexpr double PT5 = 0.5;
    for (int i = 1; i <= nfcst; ++i) {
        double corfac = fctse[i - 1] * fctse[i - 1] * PT5;
        fctcor[i - 1] =
            ltrans ? std::exp(corfac + fctunc[i - 1]) : corfac + fctunc[i - 1];
    }
}

}  // namespace x13
