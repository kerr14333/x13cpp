// spectrum_peaks.hpp -- the spectrum PEAK diagnostics (mkpeak.f / mkfreq.f /
// idpeak.f / ispeak.f / svpeak.f / smpeak.f / mxpeak.f / svfreq.f).
//
// x11ari.f:282-287 calls spcdrv under a plain `IF(Ny.eq.12)`, with no
// dependence on the `spectrum{}` spec, so the oracle computes this block on
// EVERY monthly run: 289 of the 331 `.udg` goldens carry `spcori.*`. This port
// gated run_spectrum on `spectrum{}` being present and had none of the peak
// arithmetic at all, so the whole block was silently absent.
//
// The 61-point spectra it builds on were already ported and bit-exact
// (run_spectrum.cpp, test_spectrum_tables.py); what follows is arithmetic on
// top of that, plus one extra evaluation of the same estimator on an ENHANCED
// frequency grid -- svpeak reads TWO arrays, the 61-point one for the median
// and range and a 67-point one (the base grid with each peak frequency and its
// +/-Peakwd limits inserted) that every peak test indexes.
#ifndef X13_DRIVER_SPECTRUM_PEAKS_HPP
#define X13_DRIVER_SPECTRUM_PEAKS_HPP

#include <string>
#include <vector>

namespace x13 {

struct X13Context;

// mkpeak.f, monthly + default frequencies. Every field is a constant table
// selected by Peakwd; only the indices move.
struct SpecPeakGrid {
    bool ok = false;
    int nfreq = 0;                  // the ENHANCED grid length (67 at Peakwd=1)
    std::vector<int> speak, slow, sup;   // seasonal peak / lower / upper index
    std::vector<int> tpeak, tlow, tup;   // trading-day ditto
    std::vector<double> sfreq, tfreq;    // the frequencies themselves
    std::vector<double> frqpk;           // mkfreq.f's enhanced grid
};

// One table's worth of peak diagnostics (svpeak.f + smpeak.f + mxpeak.f).
struct SpecPeakRow {
    std::string label;      // "s1".."s5" / "t1".."t2"
    bool nopeak = false;    // smpeak.f: the peak sits at or below its base
    double stars = 0.0;     // (Sxx(peak) - base) / star1
    bool above_median = false;   // the trailing '+' marker
};

struct SpecPeaks {
    std::string prefix;     // spcori / spcsa / spcirr / spcrsd
    double median = 0.0, range = 0.0;
    std::vector<SpecPeakRow> td, seas;
    std::string tdom = "no", sdom = "no", dom = "no";
    bool have_td = false;   // the TD family is skipped when Ltdfrq is off
};

// mkpeak.f + mkfreq.f -- the constant index/frequency tables and the enhanced
// grid. Returns ok=false for any configuration this increment does not cover
// (non-monthly, an explicit peakwd other than 1, altfreq, showseasonalfreq),
// so the caller can decline rather than guess.
SpecPeakGrid spectrum_peak_grid(int sp, int peakwd, bool lfqalt, bool lprsfq);

// svpeak.f -- the median/range rows and, through smpeak.f and mxpeak.f, one row
// per seasonal and trading-day frequency plus the three dominant-frequency
// labels. `sxx` is the 61-point spectrum, `sxx2` the same estimator on
// grid.frqpk.
SpecPeaks spectrum_peaks(const std::vector<double>& sxx,
                         const std::vector<double>& sxx2,
                         const SpecPeakGrid& grid, double spclim, bool ldecbl,
                         bool ltdfrq, const std::string& prefix);

}  // namespace x13

#endif  // X13_DRIVER_SPECTRUM_PEAKS_HPP
