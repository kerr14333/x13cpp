// d8bd9a.cpp -- see hpp.
#include "x11/d8bd9a.hpp"

#include "common/x13context.hpp"
#include "specparse/specparse.hpp"   // addate, dfdate, getstr
#include "regarima/outlier.hpp"      // rdotlr
#include "numeric/numeric.hpp"       // dpeq
#include "gen/model.hpp"

#include <cmath>
#include <string>
#include <vector>

namespace x13 {

namespace {

// numaff.f -- how many SI ratios a level shift of a given magnitude disturbs,
// as a function of the percent level change and the trend-filter length.
int numaff(double betals, int muladd, int nterm) {
    static const int NUMSI[12][5] = {
        {0, 0, 0, 0, 0}, {1, 1, 0, 0, 0}, {1, 1, 1, 0, 0}, {1, 1, 1, 1, 0},
        {2, 1, 1, 1, 0}, {2, 2, 1, 1, 0}, {2, 2, 1, 1, 1}, {3, 2, 1, 1, 1},
        {3, 2, 2, 1, 1}, {4, 2, 2, 1, 1}, {4, 3, 2, 1, 1}, {5, 3, 2, 1, 1}};
    static const double LIMSI[11] = {1.1, 1.2, 1.3, 1.5, 1.8, 1.9,
                                     2.0, 2.6, 2.9, 3.6, 5.5};
    if (muladd == 1) return 1;
    const double pctchg = std::fabs((1.0 / std::exp(betals) - 1.0) * 100.0);
    int itrend;
    if (nterm >= 23)      itrend = 1;
    else if (nterm >= 13) itrend = 2;
    else if (nterm >= 9)  itrend = 3;
    else if (nterm >= 7)  itrend = 4;
    else                  itrend = 5;
    for (int i = 0; i < 11; ++i)
        if (pctchg <= LIMSI[i]) return NUMSI[i][itrend - 1];
    return NUMSI[11][itrend - 1];
}

}  // namespace

void prtd9a_savelog(X13Context& ctx) {
    const int ny = ctx.x11opt.ny;
    D8bD9aOutput& out = ctx.d8bd9a;
    out.d9a_ibar.assign(static_cast<std::size_t>(ny), 0.0);
    out.d9a_sbar.assign(static_cast<std::size_t>(ny), 0.0);
    out.d9a_msr.assign(static_cast<std::size_t>(ny), 0.0);
    for (int i = 1; i <= ny; ++i) {
        out.d9a_ibar[i - 1] = ctx.x11opt.rati(i);
        out.d9a_sbar[i - 1] = ctx.x11opt.rati(i + ny);
        out.d9a_msr[i - 1] = ctx.x11opt.rati(i + 2 * ny);
    }
    out.ran_d9a = true;
}

void prtd8b_savelog(X13Context& ctx, const double* stwt, int pos1ob,
                    int posfob) {
    const int ny = ctx.x11opt.ny;
    const model_cmn& m = ctx.model;
    const x11adj_cmn& adj = ctx.x11adj;
    D8bD9aOutput& out = ctx.d8bd9a;
    out.d8b_period.clear();
    out.d8b_text.clear();
    if (posfob < pos1ob) return;

    // extchr(i) is a TWO-character label per observation; extind(i) counts the
    // reasons. Both are 1-based over the padded buffer.
    const int n = posfob + 1;
    std::vector<int> extind(static_cast<std::size_t>(n) + 1, 0);
    std::vector<std::string> extchr(static_cast<std::size_t>(n) + 1, "  ");

    // prtd8b.f:70-76 -- an X-11 extreme value is any observation the
    // extreme-value procedure did not leave at full weight.
    for (int i = pos1ob; i <= posfob; ++i)
        if (!dpeq(stwt[i - 1], 1.0)) extind[i] += 1;

    // prtd8b.f:79-126 -- regARIMA outliers, but only of the types this run
    // actually ADJUSTS for. An LS additionally marks its neighbourhood with
    // '-' directly in extchr (not through extind), so those observations are
    // reported even though nothing about them is extreme.
    const bool any_otl =
        (adj.adjls == 1 && (adj.nls > 0 || adj.nramp > 0)) ||
        (adj.nao > 0 && adj.adjao == 1) || (adj.adjtc == 1 && adj.ntc > 0);
    if (any_otl) {
        for (int icol = 1; icol <= m.nb; ++icol) {
            const int t = m.rgvrtp(icol);
            const bool want =
                (adj.adjao == 1 && (t == prm::PRGTAO || t == prm::PRGTAA)) ||
                (adj.adjtc == 1 && (t == prm::PRGTTC || t == prm::PRGTAT)) ||
                (adj.adjls == 1 &&
                 (t == prm::PRGTLS || t == prm::PRGTAL || t == prm::PRGTRP ||
                  t == prm::PRGTTL || t == prm::PRGTQD || t == prm::PRGTQI));
            if (!want) continue;

            std::string str;
            int nchr = 0;
            getstr(ctx, m.colttl.data(), m.colptr.data(), m.ncoltl, icol, str,
                   nchr);
            if (ctx.error.lfatal) return;
            int otltyp = 0, begotl = 0, endotl = 0;
            bool locok = true;
            rdotlr(ctx, str.substr(0, nchr), ctx.arima.begxy.data(), m.sp,
                   otltyp, begotl, endotl, locok);
            if (!locok || ctx.error.lfatal) return;

            int ndsp = 0;
            dfdate(ctx.arima.begxy.data(), ctx.extend.begbk2.data(), m.sp, ndsp);
            int i = begotl + ndsp;
            if (i >= 1 && i <= posfob) extind[i] += 2;
            if (otltyp == prm::RP) {
                for (int k = begotl + ndsp + 1; k <= endotl + ndsp; ++k)
                    if (k >= 1 && k <= posfob) extind[k] += 2;
            } else if (otltyp == prm::LS) {
                const int im1 = numaff(ctx.mdldat.b(icol), ctx.x11opt.muladd,
                                       ctx.x11opt.nterm);
                for (int im2 = 1; im2 <= im1; ++im2) {
                    const int hi = i + im2;
                    if (hi <= posfob && hi >= 1) extchr[hi] = " -";
                    const int lo = i - im2;
                    if (lo >= pos1ob) extchr[lo] = " -";
                }
            }
        }
    }

    // prtd8b.f:129-146 -- collapse the reason counts into one label character.
    for (int i = pos1ob; i <= posfob; ++i) {
        if (extind[i] <= 0) continue;
        if (extind[i] == 1)            extchr[i][0] = '*';
        else if (extind[i] == 2)       extchr[i][0] = '#';
        else if (extind[i] % 2 == 0)   extchr[i][0] = '@';
        else                           extchr[i][0] = '&';
    }

    // prtd8b.f:315-362 -- one row per PERIOD, listing the years that carry a
    // label. Note the row is labelled with the DATE's period (idate(MO)) as it
    // stands after the LAST addate of the inner loop, not with the loop index.
    for (int i = pos1ob; i <= pos1ob + ny - 1; ++i) {
        std::string outstr;
        int idate[2] = {0, 0};
        for (int j = i; j <= posfob; j += ny) {
            addate(ctx.extend.begbk2.data(), ny, j - 1, idate);
            if (extchr[j] == "  ") continue;
            outstr += std::to_string(idate[0]);
            if (extchr[j][0] != ' ') {
                if (extchr[j][0] == '*') {
                    // The '*' becomes 'z' when the weight went all the way to
                    // zero -- the observation was REPLACED, not just damped.
                    outstr += dpeq(stwt[j - 1], 0.0) ? 'z' : extchr[j][0];
                } else {
                    outstr += extchr[j][0];
                    if (dpeq(stwt[j - 1], 0.0)) outstr += 'z';
                }
            }
            if (extchr[j][1] != ' ') outstr += extchr[j][1];
            outstr += ' ';
        }
        out.d8b_period.push_back(idate[1]);
        out.d8b_text.push_back(outstr.empty() ? std::string("none") : outstr);
    }
    out.ran_d8b = true;
}

}  // namespace x13
