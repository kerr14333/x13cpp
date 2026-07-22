// decompspectrum.hpp -- DecompSpectrum (spectrum.f:1380-2513) + MAspectrum
// (spectrum.f:2710-2890): turn SPECTRU's Ut/V/Uc (preliminary numerator
// harmonic functions, SpectruResult::h) plus enot/estar/enoc (the
// per-component spectrum floors) into the FINAL per-component MA numerator
// polynomials + innovation variances via the already-ported MAK1. These
// (THETP/THETS/THETC/THADJ + VARWNP/VARWNS/VARWNC/VARWNA) are the actual
// source of the .mdc's sanum/saden/savar/trnum/trden/trvar keys, traced this
// session via ansub9.f's USRENTRY IFUNC=2001-2013 dispatch (called from
// ShowComp, spectrum.f:2423/2505, itself called from DecompSpectrum).
//
// PARITY:
//  - DecompSpectrum's "subtract minima" half (spectrum.f:1452-1629, padding
//    Ut/V/Uc to match Ft/Fs/Fc's length and building utf/vf/ucf) is ported,
//    AND (session 12) so is the ct/cs/cc "filter numerator" construction
//    interleaved with it (spectrum.f:1529-1533/1570-1573/1625-1628:
//    ct(1)=us(1), ct(j)=0.5*us(j) for j=2..nus, via two MULTFN calls
//    chaining utf*Fc*Fs -- and analogously cs from vf*Fc*Ft, cc from
//    ucf*Fs*Ft). ct/cs/cc are NOT dead/plotting-only code (only the
//    PLOTFILTERS() calls immediately around them are commented out in the
//    vendored source) -- SIGEX's own local ct/cs/cc (sigex.f:709-721,
//    propagated from SPECTRUM's formal params of the same name) are
//    consumed directly by ESTBUR (sigex.f:1395) as its trend/seasonal/
//    cycle numerator inputs, confirmed by instrumenting the oracle
//    (session 12, tools/seats_scope.md) and dumping ESTBUR's actual
//    runtime ct/cs/cc/pstar/qstar for unrate_seats.
//  - MAspectrum's `thadj` branch for npsi!=1 (spectrum.f:2829-2883, the
//    "real" seasonally-adjusted MAK1 factorization combining
//    Chcyc/Thetp/Thetc) IS ported for completeness but is NOT exercised or
//    gated by any current corpus spec (all 8 have npsi==1 -- no seasonal
//    structure at all, or the seasonal AR/MA/diff group is entirely absent).
//  - CORRECTION (session 12): sessions 10-11 incorrectly believed
//    `cd.pstar`/`cd.qstar` were 1 for unrate_seats and that ESTBUR needed a
//    "B-J sign switch" (sigex.f:1320-1380) applied to grow them to 2. Both
//    an oracle instrumentation dump (ESTBUR's actual runtime pstar=2,
//    qstar=2, general MLTSOL branch) AND a direct probe of this project's
//    own `cd.pstar`/`cd.qstar` (already 2, via `conv()`'s `lplus1` return --
//    `qstar`/`pstar` are LENGTHS here, not degrees) confirm `cd.pstar`/
//    `cd.qstar`/`cd.thstar` and `Totden=conv(cd.psi,cd.chcyc)` are used by
//    ESTBUR DIRECTLY, with NO switch/transform needed at all. The switch
//    block sessions 10-11 chased is real (it exists, sigex.f:924-974 and
//    1320-1380) but is irrelevant to ESTBUR's ct/cs/cc/totden/thstar/
//    pstar/qstar inputs -- whatever it does to SIGEX's OWN local copies of
//    these variables nets out to their original (pre-switch) values for
//    every case checked, and this project never carries those SIGEX locals
//    across the switch anyway (canonical_denoms.cpp computes pstar/qstar
//    once, directly, via conv()).
#ifndef X13_SEATS_DECOMPSPECTRUM_HPP
#define X13_SEATS_DECOMPSPECTRUM_HPP

#include "seats/canonical_denoms.hpp"
#include "seats/spectru.hpp"

namespace x13 {

struct SeatsComponentModels {
    double thetp[40] = {};
    int nthetp = 0;
    double varwnp = 0.0;
    double thets[40] = {};
    int nthets = 0;
    double varwns = 0.0;
    double thetc[40] = {};
    int nthetc = 0;
    double varwnc = 0.0;
    double thadj[40] = {};
    int nthadj = 0;
    double varwna = 0.0;

    // ESTBUR's own trend/seasonal/cycle filter numerators (spectrum.f:1452-
    // 1629, session 12) -- NOT the same as thetp/thets/thetc above (those
    // are the MAK1-factored, minimum-phase component MA models feeding the
    // .mdc; ct/cs/cc are the earlier, pre-factorization symmetric spectral
    // numerators ESTBUR's Tunicliffe-Wilson two-filter solve consumes
    // directly). Zero (length 0) whenever the corresponding branch guard
    // (nchi!=1 / npsi!=1 / ncycth!=0||ncyc!=1) doesn't fire, matching the
    // oracle's own ct/cs/cc(1..32)=0 initialization (spectrum.f:1452-1455).
    // Sized 80 (not 32, matching the oracle's own array -- session 12:
    // widened defensively after a buffer overflow crashed the harness on
    // specs with larger seasonal polynomials; this port's own MULTFN chain
    // can produce more than 32 coefficients for those UNVALIDATED,
    // out-of-scope-this-pass models, so the buffer is sized to this
    // function's own scratch-buffer size instead of the oracle's, which
    // apparently relies on those models never actually reaching 32 -- not
    // independently confirmed, since ct/cs/cc beyond unrate_seats are not
    // gated or trusted yet).
    double ct[80] = {};
    int nct = 0;
    double cs[80] = {};
    int ncs = 0;
    double cc[80] = {};
    int ncc = 0;
};

// decomp_spectrum -- combines DecompSpectrum's Ut/V/Uc padding + utf/vf/ucf
// construction (spectrum.f:1452-1629) with MAspectrum's MAK1 factorization
// (spectrum.f:2739-2884). Mutates `sr.h` in place (Ut/Fc/Nc get padded/
// extended, matching the oracle's own COMMON-block side effects at
// spectrum.f:1463-1464/1536/1579-1589) -- call this exactly once per
// SpectruResult, immediately after spectru(). is_close_to_td should be
// `cd.is_close_to_td` (always false in this port so far, see
// seatsalloc.hpp/f1rst.cpp -- no corpus spec sets it).
void decomp_spectrum(SpectruResult& sr, const SeatsCanonicalDenoms& cd,
                      bool is_close_to_td, SeatsComponentModels& out);

}  // namespace x13

#endif  // X13_SEATS_DECOMPSPECTRUM_HPP
