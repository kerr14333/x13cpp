// savmtx.cpp -- savmtx.f: capture a matrix table (e.g. the regression matrix,
// table 'rmx') as a /rdb save file: a Date column plus one column per
// regressor, tab-separated, values in the run's fixed sp,e-format (dtoc).
//
// Like savtbl.cpp, the text is accumulated into a SaveTable buffer that the
// driver serializes to <base>.<ext>; the numeric surface stores one
// (date, value) pair per matrix element in row-major order.
#include "tables/tables.hpp"
#include "tables/tbxdic.hpp"

#include <string>

#include "specparse/specparse.hpp"   // itoc, addate, getstr, abend

namespace x13 {

namespace {
constexpr int YR = 0, MO = 1;   // 0-based date components

std::string trim3(std::string_view ext) {
    std::size_t e = ext.find_last_not_of(' ');
    return (e == std::string_view::npos) ? std::string() : std::string(ext.substr(0, e + 1));
}
}  // namespace

// savmtx.f
void savmtx(X13Context& ctx, int itbl, const int* begxy, int sp, const double* xy,
            int nrxy, int ncxy, const char* ttlstr, const int* ttlptr, int nttl,
            const std::string& base) {
    static const char dash[] = "----------------------";   // 22 dashes

    std::string_view rawext = tbx_ext(itbl);
    std::string ext = trim3(rawext);

    SaveTable& st = ctx.saves.add();
    st.itbl = itbl;
    st.ext = ext;
    st.filename = base + "." + ext;
    st.label = "Date";

    std::string& text = st.text;
    text.reserve(static_cast<std::size_t>((nrxy + 2) * (8 + 23 * nttl)));

    // WRITE(fh,1010)'Date',(TABCHR,Ttlstr(...),ielt=1,Nttl)
    text += "Date";
    for (int ielt = 1; ielt <= nttl; ++ielt) {
        int b = ttlptr[ielt - 1];
        int e = ttlptr[ielt] - 1;
        text += '\t';
        if (e >= b) text.append(ttlstr + (b - 1), static_cast<std::size_t>(e - b + 1));
    }
    text += '\n';
    // WRITE(fh,1010)'----',(TABCHR,dash(1:Svsize),ielt=1,Nttl)
    text += "----";
    for (int ielt = 1; ielt <= nttl; ++ielt) {
        text += '\t';
        text.append(dash, static_cast<std::size_t>(ctx.savcmn.svsize));
    }
    text += '\n';

    for (int tpnt = 1; tpnt <= nrxy; ++tpnt) {
        int idate[2];
        addate(begxy, sp, tpnt - 1, idate);
        int rdbdat = (sp == 1) ? idate[YR] : 100 * idate[YR] + idate[MO];

        int begelt = ncxy * (tpnt - 1) + 1;
        int endelt = ncxy * tpnt - (ncxy - nttl);

        std::string outstr(static_cast<std::size_t>(6 + 23 * nttl + 8), ' ');
        int ipos = 1;
        itoc(ctx, rdbdat, outstr, ipos);
        if (ctx.error.lfatal) return;

        for (int ielt = begelt; ielt <= endelt; ++ielt) {
            outstr[static_cast<std::size_t>(ipos - 1)] = '\t';
            ipos = ipos + 1;
            double val = xy[ielt - 1];
            st.data.emplace_back(rdbdat, val);
            dtoc(ctx, val, outstr, ipos);
            if (ctx.error.lfatal) return;
        }
        text += outstr.substr(0, static_cast<std::size_t>(ipos - 1)) + "\n";
    }
}

}  // namespace x13
