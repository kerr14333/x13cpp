// x11force.hpp -- X-11 "force yearly totals" benchmarking (the force{} spec).
//
// These routines revise the final seasonally adjusted series (D11) so its
// calendar-year totals match a target series' yearly totals:
//   qmap  (type=denton)  -- modified-Denton proportional benchmarking, a fixed
//                           quintic-spline convolution of the annual
//                           discrepancies (ported here).
//   qmap2 (type=regress) -- Cholette-Dagum regression benchmarking (to come;
//                           needs the matrix helpers MATMLT/SIMUL/MEANCRA).
// Called from x11pt3 (Part D finals) when force{} sets Iyrt>0.
#ifndef X13_X11_X11FORCE_HPP
#define X13_X11_X11FORCE_HPP

namespace x13 {

// qmap.f: modified-Denton benchmarking of stci to series' yearly totals.
//
// Reads the target `series` and the seasonally adjusted `stci`, writes the
// revised (forced) series into `stci2`. Arrays are the engine's 1-based farrays
// passed as raw pointers (Fortran Series(j) == series[j-1]); the working span is
// the 1-based inclusive range [lfda,llda]. `ny` is the seasonal period (4 or 12),
// `nyrt` (Begyrt) the period index that begins each yearly total. `ns`/`ne` are
// OUTPUTS: the first/last observation of the first/last full year actually
// benchmarked (the caller extends the adjustment across any partial years).
void qmap(const double* series, const double* stci, double* stci2, int lfda,
          int llda, int ny, int& ns, int& ne, int nyrt);

// qmap2.f: Cholette-Dagum regression benchmarking of stci to series' yearly
// totals. Same array/span convention as qmap (1-based farrays, span
// [lfda,llda]); `ny` is the seasonal period, `begyrt` the period that begins a
// yearly total, `lamda`/`rol`/`mid` the force{} lambda/rho/mode parameters.
// Writes the forced series into stci2. `iagr` (composite aggregation flag) is
// accepted for signature parity but affects only the deferred cr/rr save tables.
// If cratio/rratio are non-null, the per-obs correction-ratio series are written
// there (force{} cr/rr tables); pass null to skip that block.
void qmap2(const double* series, const double* stci, double* stci2, int lfda,
           int llda, int ny, int iagr, double lamda, double rol, int mid,
           int begyrt, double* cratio = nullptr, double* rratio = nullptr);

// rndsa.f: round the seasonally adjusted series `sa` over [l1,l2] so each year's
// rounded values sum to the rounded annual total (UK X-11 style), writing the
// result into `sarnd`. `ny` is the seasonal period, `kdec` the number of output
// decimals. Sets rndok=false only on integer overflow (unreachable for the
// ported spans). Does not modify `sa`.
void rndsa(const double* sa, double* sarnd, int l1, int l2, int ny, int kdec,
           bool& rndok);

}  // namespace x13

#endif  // X13_X11_X11FORCE_HPP
