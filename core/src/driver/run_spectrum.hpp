// run_spectrum.hpp -- spectrum{} diagnostic compute (spcdrv.f, periodogram path).
//
// Ports the empirical (data-based) spectrum tables the spectrum{} spec saves:
// sp0 (detrended original / AdjOri), sp1 (detrended seasonally adjusted),
// sp2 (irregular). This is NOT the SEATS theoretical component spectrum
// (core/src/seats/spectru.cpp) -- it is the periodogram of the already-computed
// X-11 series (Stcsi/Stci/Sti), detrended per spcdrv.f, at 61 frequencies.
//
// Increment 1 of tools/spectrum_scope.md: periodogram type only (Spctyp=1),
// the 61-frequency save tables sp0/sp1/sp2. spr (residual spectrum), the Tukey
// tables (st0/st1/st2), and the arspec type are follow-on increments.
#ifndef X13_DRIVER_RUN_SPECTRUM_HPP
#define X13_DRIVER_RUN_SPECTRUM_HPP

#include <vector>

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
    std::vector<double> frq;  // 61 frequencies (mkfreq.f)
    std::vector<double> sp0;  // 61 -- 10*Log(Spectrum_AdjOri)
    std::vector<double> sp1;  // 61 -- 10*Log(Spectrum_SA)
    std::vector<double> sp2;  // 61 -- 10*Log(Spectrum_Irr)
    std::vector<double> spr;  // 61 -- 10*Log(Spectrum_Rsd) (regARIMA residuals)
};

// Compute the spectrum{} periodogram tables after the X-11 decomposition. No-op
// (ctx.spcout.ran stays false) when spectrum{} was not requested. Reads the
// bit-exact Stcsi/Stci/Sti and the /rho/ options captured by gt_spectrum.
bool run_spectrum(X13Context& ctx);

}  // namespace x13

#endif  // X13_DRIVER_RUN_SPECTRUM_HPP
