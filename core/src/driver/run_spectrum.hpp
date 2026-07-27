// run_spectrum.hpp -- spectrum{} diagnostic compute (spcdrv.f, periodogram path).
//
// Ports the empirical (data-based) spectrum tables the spectrum{} spec saves:
// sp0 (detrended original / AdjOri), sp1 (detrended seasonally adjusted),
// sp2 (irregular). This is NOT the SEATS theoretical component spectrum
// (core/src/seats/spectru.cpp) -- it is the periodogram of the already-computed
// X-11 series (Stcsi/Stci/Sti), detrended per spcdrv.f, at 61 frequencies.
//
// Both estimator types (arspec and periodogram), all four tables
// (sp0/sp1/sp2/spr), their Tukey twins, and the peak savelog block are here.
// See run_spectrum.cpp's header for what is still follow-on.
#ifndef X13_DRIVER_RUN_SPECTRUM_HPP
#define X13_DRIVER_RUN_SPECTRUM_HPP

#include <string>
#include <vector>

#include "driver/spectrum_peaks.hpp"

namespace x13 {

struct X13Context;

// Non-oracle-mirrored result struct: the 61-frequency grid and the three
// periodogram save tables, surfaced for the parity harness to emit.
struct SpectrumOutput {
    bool requested = false;   // spectrum{} block present (set by gt_spectrum)
    bool ran = false;         // run_spectrum executed and produced tables
    bool have_sp0 = false;
    bool have_sp1 = false;
    bool have_sp2 = false;
    bool have_spr = false;
    bool have_st0 = false;
    bool have_st1 = false;
    bool have_st2 = false;
    std::vector<double> frq;  // 61 frequencies (mkfreq.f)
    std::vector<double> sp0;  // 61 -- 10*Log(Spectrum_AdjOri)
    std::vector<double> sp1;  // 61 -- 10*Log(Spectrum_SA)
    std::vector<double> sp2;  // 61 -- 10*Log(Spectrum_Irr)
    std::vector<double> spr;  // 61 -- 10*Log(Spectrum_Rsd) (regARIMA residuals)
    // Tukey-smoothed spectra of the same AdjOri/SA/Irr series (getTPeaks/covWind).
    // Their grid is Frq(i)=i/m over i=0..m/2 (m = Tukey window, not the 61-pt grid).
    std::vector<double> frq_tukey;  // m/2+1 frequencies i/m
    std::vector<double> st0;        // Tukey(Spectrum_AdjOri)
    std::vector<double> st1;        // Tukey(Spectrum_SA)
    std::vector<double> st2;        // Tukey(Spectrum_Irr)
    // svpeak.f / svfreq.f / savpk.f -- the peak canaries. One `peaks` entry per
    // spectrum table, in the order the oracle emits them.
    SpecPeakGrid grid;
    std::vector<SpecPeaks> peaks;
    // savpk.f -- the accumulated `peaks.seas` / `peaks.td` label lists
    // (spcdrv.f:750-794), "none" when the family found no visually significant
    // peak in any table.
    std::string peaks_seas, peaks_td;
    // getTPeaks' peak PROBABILITIES (`spcXXX.tukey.m/.s1-.s6/.td`), one entry
    // per Tukey table in Itukey order -- `rsd` first, because spcrsd.f files its
    // entry during the regARIMA phase, before spcdrv runs at all.
    std::vector<TukeyEntry> tukey;
    // svtukp.f -- the four accumulated `peaks.tukey.*` label lists.
    TukeyLabels tukey_labels;
};

// Compute the spectrum diagnostics after the X-11 decomposition. NOT gated on
// spectrum{} being present: x11ari.f:282-287 calls spcdrv under a plain
// IF(Ny.eq.12), so the oracle produces this block on every monthly run. Reads
// the bit-exact Stcsi/Stci/Sti and the /rho/ options gtinpt defaults.
bool run_spectrum(X13Context& ctx);

}  // namespace x13

#endif  // X13_DRIVER_RUN_SPECTRUM_HPP
