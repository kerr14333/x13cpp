// savtbl.cpp -- savtbl.f / punch.f: capture a table as a /rdb save file.
//
// The table is captured BOTH numerically (into ctx.saves, the library data
// surface) and as byte-identical /rdb text (date<TAB>value, fixed sp,e-format).
// The Fortran opens a real file via opnfil.f and writes with WRITE(fh,...); here
// we accumulate the text into a SaveTable buffer that the driver serializes to
// <base>.<ext> exactly as the CLI would. The graphics-directory path (Lgraf)
// and the file-table log side effects of opnfil.f are out of scope.
#include "tables/tables.hpp"
#include "tables/tbxdic.hpp"

#include <string>
#include <vector>

#include "specparse/specparse.hpp"   // itoc, addate, abend

namespace x13 {

namespace {
constexpr int YR = 0, MO = 1;   // 0-based date components

std::string trim3(std::string_view ext) {
    std::size_t e = ext.find_last_not_of(' ');
    return (e == std::string_view::npos) ? std::string() : std::string(ext.substr(0, e + 1));
}
}  // namespace

// savtbl.f
void savtbl(X13Context& ctx, int itbl, const int* begdat, int i1, int na, int sp,
            const double* avec, const std::string& base, const std::string& label,
            int nser) {
    std::string_view rawext = tbx_ext(itbl);   // e.g. "a1 " (3 chars, trailing pad kept)
    std::string ext = trim3(rawext);            // "a1"

    SaveTable& st = ctx.saves.add();
    st.itbl = itbl;
    st.ext = ext;
    st.filename = base + "." + ext;

    // Header label: Label(1:Nser)//'.'//tbxdic(Itbl)  (the 3-char, space-padded ext).
    std::string tmplbl = label.substr(0, static_cast<std::size_t>(nser)) + "." +
                         std::string(rawext);
    st.label = tmplbl;

    std::string& text = st.text;
    text.reserve(static_cast<std::size_t>((na - i1 + 3) * 32));

    // WRITE(fh,1010)'date',TABCHR,tmplbl(1:ns4)   FORMAT(a:,a,a)
    text += "date\t" + tmplbl + "\n";
    // WRITE(fh,1010)'------',TABCHR,'-----------------------'
    text += "------\t-----------------------\n";

    for (int tpnt = i1; tpnt <= na; ++tpnt) {
        int idate[2];
        addate(begdat, sp, tpnt - 1, idate);
        int rdbdat = (sp == 1) ? idate[YR] : 100 * idate[YR] + idate[MO];

        double val = avec[static_cast<std::size_t>(tpnt - 1)];   // avec is 1-based
        st.data.emplace_back(rdbdat, val);

        std::string outstr(30, ' ');
        int ipos = 1;
        itoc(ctx, rdbdat, outstr, ipos);
        if (ctx.error.lfatal) return;
        outstr[static_cast<std::size_t>(ipos - 1)] = '\t';
        ++ipos;
        dtoc(ctx, val, outstr, ipos);
        if (ctx.error.lfatal) return;
        text += outstr.substr(0, static_cast<std::size_t>(ipos - 1)) + "\n";
    }
}

// punch.f (Lgraf graphics path omitted; Lhiddn transparency handled by caller)
void punch(X13Context& ctx, const double* x, int mfda, int mlda, int itbl,
           const int* begdat, int sp, const std::string& base,
           const std::string& label, int nser, bool lpct) {
    std::vector<double> y(static_cast<std::size_t>(mlda));
    for (int i = mfda; i <= mlda; ++i)
        y[static_cast<std::size_t>(i - 1)] = lpct ? x[static_cast<std::size_t>(i - 1)] * 100.0
                                                  : x[static_cast<std::size_t>(i - 1)];
    savtbl(ctx, itbl, begdat, mfda, mlda, sp, y.data(), base, label, nser);
}

}  // namespace x13
