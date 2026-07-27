// d8bd9a.hpp -- the D8B and D9A savelog blocks (prtd8b.f / prtd9a.f).
//
// Two X-11 diagnostics the port computed everything for and then never
// reported: both call sites in x11pt3 were marked "deferred" alongside their
// print tables, but each also writes a `.udg` savelog block that is not print
// surface. 172 goldens carry `d8b.NN`, 169 carry `d9a.NN`.
//
//  * D9A -- the moving-seasonality-ratio table: per period, the average
//    year-to-year change of the irregular (Ibar), of the seasonal (Sbar), and
//    their ratio. `vsfa` already computes all three into /x11opt/ Rati; only
//    the report was missing.
//  * D8B -- which YEARS of each period carry an "extreme" SI ratio, and why:
//    `*` (down-weighted by the X-11 extreme-value procedure), `z` (weight
//    driven to zero), `#`/`@`/`&` (a regARIMA outlier, alone or combined with
//    an extreme), and `-` (an observation near a level shift).
#ifndef X13_X11_D8BD9A_HPP
#define X13_X11_D8BD9A_HPP

#include <string>
#include <vector>

namespace x13 {

struct X13Context;

struct D8bD9aOutput {
    bool ran_d8b = false;
    bool ran_d9a = false;
    // d8b.NN -> the space-separated year+flag list ("1950* 1953z"), or "none".
    // Indexed 0..ny-1 by PERIOD, with the period number the row is labelled
    // with kept alongside (prtd8b labels by the DATE, not the loop index).
    std::vector<int> d8b_period;
    std::vector<std::string> d8b_text;
    // d9a.NN -> Rati(i), Rati(i+Ny), Rati(i+2*Ny)
    std::vector<double> d9a_ibar, d9a_sbar, d9a_msr;
};

// prtd8b.f:63-146 + :315-362 -- the extreme/outlier labels and their savelog
// rows. Reads the UNMODIFIED SI ratios' weights (Stwt) and the regression
// design; writes ctx.d8bd9a.
void prtd8b_savelog(X13Context& ctx, const double* stwt, int pos1ob, int posfob);

// prtd9a.f:31-37 -- the moving-seasonality-ratio rows, straight off Rati.
void prtd9a_savelog(X13Context& ctx);

}  // namespace x13

#endif  // X13_X11_D8BD9A_HPP
