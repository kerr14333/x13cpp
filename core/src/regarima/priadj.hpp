// priadj.hpp -- prior-adjustment factors (M2).
//
// Length-of-period / leap-year prior-adjustment factors (the predefined
// adjust=lom/loq/lpyear priors). These factors are deterministic functions of
// the calendar date -- no estimation is involved -- so they are reachable in the
// pre-model phase. Ported from the multiplicative branch of td7var.f (the
// seventh trading-day / length-of-period variable), which adjsrs.f calls with
// Mltadd=.true. to build the prior-factor series Adj.
#ifndef X13_REGARIMA_PRIADJ_HPP
#define X13_REGARIMA_PRIADJ_HPP

namespace x13 {

// td7var.f leap-year test: (year not a century and divisible by 4) or divisible
// by 400. Returns 2 for a leap year, 1 otherwise (the lpyr index into the tables).
int lpyr_index(int year);

// Multiplicative length-of-period prior factor for one observation (td7var.f,
// Mltadd=.true., Ltdstk=.false.):
//   sp==12 (monthly):
//     lom==true  -> days_in_month / 30.4375
//     lom==false -> Feb: 28/28.25 (or 29/28.25 leap); other months 1
//   sp==4 (quarterly):
//     lom==true  -> days_in_quarter / 91.3125
//     lom==false -> Q1: 90/90.25 (or 91/90.25 leap); other quarters 1
// `lom` is true for a length-of-month/quarter prior (Priadj 2/3), false for a
// leap-year prior (Priadj 4).
double lpfac(int year, int period, int sp, bool lom);

}  // namespace x13

#endif  // X13_REGARIMA_PRIADJ_HPP
