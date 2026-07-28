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
    // idpeak.f -- how many frequencies in each family cleared ispeak's tests.
    // These are what spcdrv.f:750-794 turns into the `peaks.seas`/`peaks.td`
    // label lists, and they are NOT derivable from the rows above: ispeak's
    // test is a different (stricter) one than smpeak's star height.
    int ltdpk = 0, lsapk = 0;
};

// mkpeak.f + mkfreq.f -- the constant index/frequency tables and the enhanced
// grid. Covers monthly data at every `peakwd` 1..4, with or without
// `showseasonalfreq` (which mkpeak does not take -- it changes mkfreq's PLOTTED
// grid only). Returns ok=false for non-monthly data and for `altfreq`, where
// mkpeak.f leaves Tpeak(3)/Tup(3) unassigned at peakwd > 1 (CB-30), so the
// caller declines rather than guesses.
SpecPeakGrid spectrum_peak_grid(int sp, int peakwd, bool lfqalt, bool lprsfq);

// svpeak.f -- the median/range rows and, through smpeak.f and mxpeak.f, one row
// per seasonal and trading-day frequency plus the three dominant-frequency
// labels. `sxx` is the 61-point spectrum, `sxx2` the same estimator on
// grid.frqpk.
SpecPeaks spectrum_peaks(const std::vector<double>& sxx,
                         const std::vector<double>& sxx2,
                         const SpecPeakGrid& grid, double spclim, bool ldecbl,
                         bool ltdfrq, double plocal, int sp,
                         const std::string& prefix);

// --- the Tukey half (specpeak.f) ----------------------------------------
//
// getTPeaks is getWind(Tukey) + covWind + Tpeaks2. The first two are already
// `tukey_spectrum` in run_spectrum.cpp (they produce the st0/st1/st2 save
// tables, bit-exact); only Tpeaks2 was missing, which is where the peak
// PROBABILITIES come from.
//
// Unlike the AR-spectrum peaks above, these are not star heights against a
// median: each candidate frequency is scored by an F test on the ratio of its
// own spectral ordinate to its neighbours', with degrees of freedom
// interpolated from the window size and the series length (dfPeaks).
struct TukeyPeaks {
    bool ok = false;      // false when the series is too short for any window
    int m = -1;           // the Tukey window (120 / 112 / 79 / 44)
    double ptd = 0.0;     // P(trading-day peak)
    double ps[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};  // P(seasonal peak k)
    double mv[14] = {0.0};  // the wide-peak test values (computed, unemitted)
};

// specpeak.f:333 -- the four F degrees of freedom for a given window/length.
// m==79 uses four constants; every other window interpolates a quadratic in
// nz/100 and 100/nz from its own 3x4 table.
void df_peaks(int m, int nz, double& df1, double& df2, double& df3,
              double& df4);

// specpeak.f:400 -- score every candidate peak of the RAW (not decibel) Tukey
// spectrum `h`, 1-based over h[0..m/2]. `nz` is the series length the spectrum
// was built from.
TukeyPeaks tpeaks2(const double* h, int m, int mq, int nz);

// svtukp.f -- the four accumulated label lists. `entries` is the Itukey order
// (spcrsd files its entry during the regARIMA phase, so `rsd` comes FIRST).
// `lsadj` is x11ari.f:76's `Lx11.or.Lseats`.
struct TukeyEntry {
    std::string label;   // "rsd" / "ori" / "sa" / "irr"
    TukeyPeaks pk;
};
struct TukeyLabels {
    std::string seas = "none", td = "none";
    std::string p90_seas = "none", p90_td = "none";
};
TukeyLabels tukey_peak_labels(const std::vector<TukeyEntry>& entries,
                              bool lsadj);

}  // namespace x13

#endif  // X13_DRIVER_SPECTRUM_PEAKS_HPP
