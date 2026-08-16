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

// NOTE ON THE PARAMETER NAME: `begspn` is what BOTH call sites pass as
// adjsrs.f:39's BEGADJ (Begspn shifted back Nbcst) and `n` is its NADJ. The
// name is a leftover and it cost something: the body used to re-derive
// `begadj = begspn - Nbcst` from it, subtracting the backcasts a second time.
// That was invisible for as long as the only paths reaching it had Nbcst == 0
// or were walled -- the addadj shift below is exactly the wall that hid it.
bool adjsrs_factors(X13Context& ctx, const int* begadj, int sp, int n,
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
    // would divide by zero.
    struct { int* n; double* v; int* beg; int* frst; } sets[2] = {
        {&ctx.priusr.nuspad, ctx.priadj.usrpad.data(),
         ctx.priusr.bgupad.data(), &ctx.priusr.frstap},
        {&ctx.priusr.nustad, ctx.priadj.usrtad.data(),
         ctx.priusr.bgutad.data(), &ctx.priusr.frstat},
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

    // addadj.f:42-57 -- Frstad is the DISPLACEMENT of the user series inside the
    // prior span, `dfdate(Begadj,Bgusra)` = Begadj - Bgusra.
    //
    //   Frstad < 0  the user factors start AFTER the prior span does, which the
    //               Fortran's own comment says to read as "this is due to
    //               backcasts": the series is shifted RIGHT by |Frstad| and the
    //               opened head filled with Base, then Nusrad grows and Bgusra
    //               is re-anchored at Begadj so the displacement is now 0. This
    //               is the ONLY writer that moves Bgutad/Bgupad, and moving it
    //               is what makes mkback's coverage test pass -- see the dead-
    //               code proof in forecast.cpp's bcstout.
    //   Frstad > 0  the user factors start BEFORE the prior span; nothing moves,
    //               the combine below just reads from `iprd + Frstad`.
    //
    // The percent -> ratio conversion that the Fortran does inside both arms is
    // NOT here: this port does it once at parse (readers_spec.cpp:316), because
    // adjsrs runs once in the oracle (editor.f:849) and twice here.
    int frstad_of[2] = {0, 0};
    for (int k = 0; k < 2; ++k) {
        const auto& s = sets[k];
        if (*s.n <= 0) continue;
        int frstad = 0;
        dfdate(begadj, s.beg, sp, frstad);
        if (frstad < 0) {
            const int shift = -frstad;
            for (int iprd = *s.n; iprd >= 1; --iprd) {
                s.v[iprd + shift - 1] = s.v[iprd - 1];
                if (iprd <= shift) s.v[iprd - 1] = base;
            }
            *s.n += shift;                            // addadj.f:50
            frstad = 0;                               // addadj.f:51
            s.beg[0] = begadj[0];                     // addadj.f:52 cpyint
            s.beg[1] = begadj[1];
        }
        frstad_of[k] = frstad;
        *s.frst = frstad + 1;                         // addadj.f:92
        // addadj.f:84-86 -- the ELSE arm of the combine loop: every slot the
        // combine would have read past the end of the user series is set to
        // Base, which is the pad x11pt3 later divides by.
        for (int iprd = 1; iprd <= n; ++iprd)
            if (iprd + frstad > *s.n) s.v[iprd + frstad - 1] = base;
    }

    for (int tpnt = 1; tpnt <= n; ++tpnt) {
        double f = 1.0;
        if (has_predef && !suppress_predef) {
            int idate[2];
            addate(begadj, sp, tpnt - 1, idate);
            f *= lpfac(idate[0], idate[1], sp, lom);   // td7var factor
        }
        // addadj.f:61-83 -- fold in the user factors (ratio mode).
        // adjsrs.f:89-97 applies BOTH sets to the same Adj, in Prtype order.
        for (int k = 0; k < 2; ++k) {
            const auto& s = sets[k];
            const int iprd2 = tpnt + frstad_of[k];
            if (*s.n > 0 && iprd2 <= *s.n) f *= s.v[iprd2 - 1];
        }
        fac[tpnt - 1] = f;
    }
    return true;
}

}  // namespace x13
