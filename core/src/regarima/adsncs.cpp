// adsncs.cpp -- adsncs.f: trigonometric (sine-cosine) seasonal-effect regressors
// into Xy columns begcol..endcol. The i-th column is sin(2*h*pi*t/sp) or
// cos(2*h*pi*t/sp), where the harmonic h and the sin/cos flag are read from the
// column title ("sin(2pi*<h>t/<sp>)" / "cos(2pi*<h>t/<sp>)"; the '3' prefix picks
// sin vs cos, the harmonic starts at character 9 -- built in getreg_vars.cpp).
#include "regarima/regvar.hpp"
#include "specparse/specparse.hpp"
#include "srslen.hpp"
#include "gen/model.hpp"

#include <cmath>
#include <string>
#include <string_view>
#include <vector>

namespace x13 {

void adsncs(X13Context& ctx, const int* begdat, int numrxy, int numcxy,
            int begcol, int endcol, double* xy, const bool* begrgm) {
    constexpr double PI = 3.14159265358979, TWO = 2.0, ZERO = 0.0;

    model_cmn& M = ctx.model;
    const int nb = numcxy - 1;
    const int ncol = endcol - begcol + 1;

    // 1 <= begcol <= endcol <= nb (adsncs.f:63-70).
    if (begcol < 1 || endcol > numcxy - 1 || endcol < begcol) {
        errhdr(ctx);
        writln(ctx, " Column error, 1<=begcol<=endcol<=nb", stdio::STDERR,
               ctx.units.mt2, true);
        abend(ctx);
        return;
    }

    // Harmonic factors 2*pi*h/sp and the sin/cos flag, parsed from each title.
    const double fac = TWO * PI / M.sp;
    const int precol = begcol - 1;
    std::vector<double> harmc(static_cast<std::size_t>(ncol));
    std::vector<char> lsin(static_cast<std::size_t>(ncol));
    for (int j = 1; j <= ncol; ++j) {
        const int i = precol + j;
        std::string str;
        int nchr = 0;
        getstr(ctx, M.colttl.data(), M.colptr.data(), nb, i, str, nchr);
        if (ctx.error.lfatal) return;
        lsin[j - 1] = (str.compare(0, 3, "sin") == 0) ? 1 : 0;
        int ipos = 9;  // harmonic digit starts at char 9 (after "sin(2pi*")
        harmc[j - 1] = fac * ctoi(std::string_view(str).substr(
                                      0, static_cast<std::size_t>(nchr)), ipos);
    }

    // premo = the month two periods before the series start (adsncs.f:93-94):
    // period 1 (each January) is the sin(0)/cos(0) row regardless of the start.
    int predat[2];
    addate(begdat, M.sp, -2, predat);
    const int premo = predat[1];

    auto XY = [&](int c, int r) -> double& {
        return xy[static_cast<std::size_t>((r - 1) * numcxy + (c - 1))];
    };

    for (int i = 1; i <= numrxy; ++i) {
        const double dmo = static_cast<double>((premo + i) % M.sp);
        for (int j = 1; j <= ncol; ++j) {
            const double rdns = harmc[j - 1] * dmo;
            XY(precol + j, i) = ZERO;
            if (begrgm[i - 1])
                XY(precol + j, i) = lsin[j - 1] ? std::sin(rdns) : std::cos(rdns);
        }
    }
}

}  // namespace x13
