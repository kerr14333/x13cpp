// priadj.cpp -- length-of-period / leap-year prior factors (td7var.f, Mltadd),
// and adjsrs.f's combination of those with the user's permanent prior factors.
#include "regarima/priadj.hpp"

#include "common/x13context.hpp"
#include "specparse/specparse.hpp"   // addate, dfdate, errhdr, writln, abend

namespace x13 {

namespace {
// td7var.f DATA lnomo/lnoqtr: days per month/quarter, [period][lpyr-1].
constexpr int LNOMO[12][2] = {{31, 31}, {28, 29}, {31, 31}, {30, 30},
                              {31, 31}, {30, 30}, {31, 31}, {31, 31},
                              {30, 30}, {31, 31}, {30, 30}, {31, 31}};
constexpr int LNOQTR[4][2] = {{90, 91}, {91, 91}, {92, 92}, {92, 92}};

constexpr double AVEMO = 30.4375, AVEQTR = 91.3125;
constexpr double P28FEB = 28.0 / 28.25, P29FEB = 29.0 / 28.25;
constexpr double P90QTR = 90.0 / 90.25, P91QTR = 91.0 / 90.25;
}  // namespace

int lpyr_index(int year) {
    bool leap = ((year % 100 != 0 && year % 4 == 0) || year % 400 == 0);
    return leap ? 2 : 1;
}

double lpfac(int year, int period, int sp, bool lom) {
    int lpyr = lpyr_index(year);   // 1 or 2

    if (sp != 12) {
        // Quarterly (td7var.f Isp!=12 branch, Mltadd=.true.).
        if (lom) {
            int ndoqtr = LNOQTR[period - 1][lpyr - 1];
            return static_cast<double>(ndoqtr) / AVEQTR;
        }
        if (period != 1) return 1.0;
        return (lpyr == 2) ? P91QTR : P90QTR;
    }

    // Monthly (td7var.f Isp==12 branch, Mltadd=.true.).
    if (lom) {
        int ndomo = LNOMO[period - 1][lpyr - 1];
        return static_cast<double>(ndomo) / AVEMO;
    }
    if (period != 2) return 1.0;
    return (lpyr == 2) ? P29FEB : P28FEB;
}

bool adjsrs_factors(X13Context& ctx, const int* begspn, int sp, int n,
                    bool suppress_predef, double* fac) {
    const int priadj = ctx.prior.priadj;
    const bool has_predef = (priadj > 1);          // 2 lom / 3 loq / 4 lpyear
    const bool lom = (priadj == 2 || priadj == 3); // adjsrs.f: lom for lom/loq
    const int nuspad = ctx.priusr.nuspad;
    const int nustad = ctx.priusr.nustad;
    const double base = (ctx.adj.adjmod == 2) ? 0.0 : 1.0;

    // addadj.f:42/92 -- where each user factor series starts relative to the
    // prior span (Begadj = Begspn shifted back Nbcst), and the pad-out of its
    // tail to Nadj (addadj.f:85-86). The pad is not cosmetic: x11pt3 divides D11
    // and D13 by these arrays over the WHOLE padded span, so an unfilled tail
    // would divide by zero. Only the aligned case (Frstad==0) is ported.
    struct { int n; double* v; int* beg; int* frst; } sets[2] = {
        {nuspad, ctx.priadj.usrpad.data(), ctx.priusr.bgupad.data(),
         &ctx.priusr.frstap},
        {nustad, ctx.priadj.usrtad.data(), ctx.priusr.bgutad.data(),
         &ctx.priusr.frstat},
    };
    // editor.f:744-762 resolves Percnt/Adjmod for the user factor sets: on an
    // ADDITIVE run an unset mode means DIFFERENCES (Percnt=2, Adjmod=2), and a
    // percent/ratio set is rejected outright. Only the multiplicative (Adjmod=1)
    // combination is ported, so both additive branches fatal here rather than
    // combining ratio factors into an additive adjustment.
    if ((nuspad > 0 || nustad > 0) && ctx.x11opt.muladd == 1) {
        errhdr(ctx);
        writln(ctx, "ERROR: additive prior adjustment factors (transform mode=diff "
                    "/ Adjmod=2) not yet ported.", stdio::STDERR, ctx.units.mt2,
               true);
        abend(ctx);
        return false;
    }

    int begadj[2];
    addate(begspn, sp, -ctx.extend.nbcst, begadj);   // adjsrs.f:39
    for (const auto& s : sets) {
        if (s.n <= 0) continue;
        int frstad = 0;
        dfdate(begadj, s.beg, sp, frstad);
        if (frstad != 0) {
            errhdr(ctx);
            writln(ctx, "ERROR: addadj user-prior span shift (Frstad!=0) not yet "
                        "ported.", stdio::STDERR, ctx.units.mt2, true);
            abend(ctx);
            return false;
        }
        *s.frst = 1;                                  // addadj.f:92 Frstad+1
        for (int i = s.n; i < n; ++i) s.v[i] = base;  // addadj.f:85-86
    }

    for (int tpnt = 1; tpnt <= n; ++tpnt) {
        double f = 1.0;
        if (has_predef && !suppress_predef) {
            int idate[2];
            addate(begspn, sp, tpnt - 1, idate);
            f *= lpfac(idate[0], idate[1], sp, lom);   // td7var factor
        }
        // addadj.f:61-87 -- fold in the user factors (ratio mode, Frstad==0).
        // adjsrs.f:89-97 applies BOTH sets to the same Adj, in Prtype order.
        for (const auto& s : sets)
            if (s.n > 0 && tpnt <= s.n) f *= s.v[tpnt - 1];
        fac[tpnt - 1] = f;
    }
    return true;
}

}  // namespace x13
