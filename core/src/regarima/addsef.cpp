// addsef.cpp -- addsef.f: seasonal-effect contrast variables (1 in month i,
// -1 in the sp-th month) into Xy columns begcol..endcol.
#include "regarima/regvar.hpp"
#include "specparse/specparse.hpp"
#include "notset.hpp"
#include "srslen.hpp"
#include "gen/model.hpp"

namespace x13 {

void addsef(X13Context& ctx, const int* begdat, int numrxy, int numcxy,
            int begcol, int endcol, double* xy, const bool* begrgm) {
    using namespace prm;
    constexpr double ZERO = 0.0, MONE = -1.0, ONE = 1.0;

    static const char MONDIC[] = "janfebmaraprmayjunjulaugsepoctnov";
    static const int monptr[12] = {1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34};
    constexpr int PMON = 11;

    model_cmn& M = ctx.model;
    int ncol = endcol - begcol + 1;
    int sp2 = ncol + 1;

    if (begcol < 1 || endcol > numcxy - 1 || endcol < begcol) {
        errhdr(ctx);
        writln(ctx, " Column error, 1<=begcol<=endcol<=nb", stdio::STDERR,
               ctx.units.mt2, true);
        abend(ctx);
        return;
    }

    int tcol[PSP];
    if (sp2 < M.sp) {
        int ipos = 1;
        setint(prm::NOTSET, M.sp - 1, tcol);
        for (int icol = begcol; icol <= endcol; ++icol) {
            std::string str;
            int nchr;
            getstr(ctx, M.colttl.data(), M.colptr.data(), M.ncoltl, icol, str, nchr);
            if (ctx.error.lfatal) return;
            int iper;
            if (M.sp == 12)
                iper = strinx(false, MONDIC, monptr, 1, PMON,
                              std::string_view(str).substr(0, 3));
            else
                iper = ctoi(str, ipos);
            tcol[iper - 1] = icol - begcol + 1;
        }
    }

    int predat[2];
    addate(begdat, M.sp, -1, predat);
    int premo = predat[1];
    int precol = begcol - 1;

    auto XY = [&](int c, int r) -> double& {
        return xy[static_cast<std::size_t>((r - 1) * numcxy + (c - 1))];
    };

    for (int i = 1; i <= numrxy; ++i) {
        int imo = (premo + i) % M.sp;
        for (int j = begcol; j <= endcol; ++j) XY(j, i) = ZERO;
        if (begrgm[i - 1]) {
            if (imo == 0) {
                for (int j = begcol; j <= endcol; ++j) XY(j, i) = MONE;
            } else {
                if (M.sp == sp2) XY(precol + imo, i) = ONE;
                else if (tcol[imo - 1] != prm::NOTSET) XY(precol + tcol[imo - 1], i) = ONE;
            }
        }
    }
}

}  // namespace x13
