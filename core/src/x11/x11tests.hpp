// x11tests.hpp -- the X-11 seasonality test battery: the two analysis-of-
// variance F-tests (stable / moving seasonality), the Kruskal-Wallis
// nonparametric equivalent, and the combined identifiable-seasonality verdict.
//
// These are what the .udg savelog reports as f2.fsb1 / f2.fsd8 / f2.kw /
// f2.msf / f2.idseasonal, and -- via /tests/ Test1,Test2 -- what feeds M7 and
// hence the F3 Q statistic. Faithful ports of ftest.f, kwtest.f, mstest.f and
// combft.f; every WRITE/print branch is dropped (this port emits no files), so
// only the COMMON-block side effects survive.
//
// Index convention: 0-based C pointers, Fortran index i -> element [i-1];
// range args stay Fortran 1-based.
#ifndef X13_X11_X11TESTS_HPP
#define X13_X11_X11TESTS_HPP

namespace x13 {

struct X13Context;

// ftest.f: one-way analysis of variance on x over [ib,ie] with nyr seasons.
//
// ind selects both the differencing and where the result lands:
//   0 -> no differencing; result to /tests/ Fstabl,P1  (the D8 stable-seasonality
//        F, savelog f2.fsd8)
//   1 -> first-difference at lag nyr/4 first; result kept local (the residual-
//        seasonality test on the SA series -- print/save only, so a no-op here)
//   2 -> no differencing; result to /tests/ Fpres,P3   (the B1 test, f2.fsb1)
//
// Returns without touching /tests/ when Issap==2 and ind>0, when ind==1 and
// x11msc Same is set, or when the residual mean square is exactly zero.
void ftest(X13Context& ctx, const double* x, int ib, int ie, int nyr, int ind);

// kwtest.f: Kruskal-Wallis test for stable seasonality. Sets /tests/ Chikw,P5.
//
// WARNING (the oracle says so too): x is DESTROYED -- it is sorted in place over
// [ib,ie]. Callers rebuild it afterwards.
void kwtest(X13Context& ctx, double* x, int ib, int ie, int nyr);

// mstest.f: two-way analysis of variance for MOVING seasonality on the
// |deviation from the mode identity| of array over [jfda,jlda]. Sets /tests/
// Fmove,P2 (savelog f2.msf), and Ssmf(Icol) under sliding spans.
void mstest(X13Context& ctx, const double* array, int jfda, int jlda, int nyr);

// combft.f: combined test for identifiable seasonality. Reads Fstabl/Fmove/
// P1/P2/P5, writes /tests/ Test1,Test2 (the M7 inputs) and Iqfail (1=yes,
// 2=no -- savelog f2.idseasonal).
void combft(X13Context& ctx);

}  // namespace x13

#endif  // X13_X11_X11TESTS_HPP
