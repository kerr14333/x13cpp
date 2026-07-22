// spectru.hpp -- SPECTRU (oracle/fortran/spectrum.f:558-1190), the real
// PARFRA/MAK1-driving canonical-decomposition routine SIGEX calls once (via
// the outer SPECTRUM wrapper, spectrum.f:82/299), plus the numeric
// minimization stack it depends on (FUNC0/Fbis/MINIM/MINIMbis/GlobalMinim/
// MinimGrid, all ansub2.f) and ADDJ (spectrum.f:3138, weighted polynomial
// add).
//
// SPECTRU partitions the model's total pseudo-spectrum F(x)/H(x) among
// trend/seasonal/cycle via PARFRA (already ported, seatsfact.hpp) when more
// than one component is nontrivial, or a direct assignment when only one is;
// then, for each nontrivial component, 1-D-minimizes its own spectrum
// function over frequency to find the flat "irregular" floor
// (enot/enoc/estar) subtracted out downstream; qt1 = qt(1)+enot+estar+enoc
// is the admissibility number (negative => DECOMPOSITION INVALID).
//
// STATE MODEL: the oracle threads Ut/Nut/Ft/Nt (func2.i), Uc/Nuc/Fc/Nc
// (func3.i), V/Nv/Fs/Ns (func.i), Ff/Nf/Fh/Nh (func4.i), Dum/Dum1 (func5.i)
// through COMMON, read by FUNC0 via the also-COMMON Ifunc selector (test.i).
// SPECTRU's own Us/Vn/Fn/NN formal *arguments* are Fortran name-collisions
// with local debug-residual temporaries (same identifier, case-insensitive)
// used only in the (out==0 .and. har==1) print branch -- not meaningful
// outputs. This port keeps the COMMON-equivalent state (SpectruHarmonics)
// explicit and drops the unused debug-only Us/Vn/Fn passthrough.
//
// PARITY: MINIM/MINIMbis/GlobalMinim/MinimGrid are transcribed with C++
// goto/labels mirroring the Fortran control flow verbatim (same technique as
// roots.cpp's C02AEF) rather than restructured -- the step-halving/
// extrapolation logic is iteration-count- and floating-comparison-sensitive.
#ifndef X13_SEATS_SPECTRU_HPP
#define X13_SEATS_SPECTRU_HPP

namespace x13 {

// The oracle's /func/../func5/ COMMON state FUNC0 reads via the Ifunc
// selector. Sizes match the Fortran DIMENSIONs exactly (func.i's V(50)/
// FS(27), func2.i's UT(22)/FT(8), func3.i's UC(32)/FC(32), func4.i's
// FF(32)/FH(32), func5.i's DUM(80)/DUM1(80)).
struct SpectruHarmonics {
    double v[50] = {};
    int nv = 0;
    double fs[27] = {};
    int ns = 0;
    double ut[22] = {};
    int nut = 0;
    double ft[8] = {};
    int nt = 0;
    double uc[32] = {};
    int nuc = 0;
    double fc[32] = {};
    int nc = 0;
    double ff[32] = {};
    int nf = 0;
    double fh[32] = {};
    int nh = 0;
    double dum[80] = {};
    int ndum = 0;
    double dum1[80] = {};
    int ndum1 = 0;
};

// FUNC0(x) -- ansub2.f:595. ifunc selects which component's spectrum ratio
// is evaluated: 1=seasonal (V/Fs), 3=cycle (Uc/Fc), 4=full model (Ff/Fh),
// 5=Dum/Dum1 ratio, else (2, or any other)=trend (Ut/Ft).
double func0(const SpectruHarmonics& h, int ifunc, double x);

// Fbis(w,haydif,mq,tol) -- ansub2.f:1380. FUNC0 with the trend (ifunc==2)
// near-zero-frequency discontinuity patched: for w<epsilon(tol degrees) and
// haydif!=0 (differencing present), evaluates at epsilon instead of w.
double fbis(const SpectruHarmonics& h, int ifunc, double w, int haydif, int mq,
            int tol);

// MINIM -- ansub2.f:729. Quadratic-interpolation line search over FUNC0
// bounded to [lb,ub], starting at `start` with step `step`, converging to
// within `dstop`. iconv=1 on non-convergence (50-iteration cap).
void minim(const SpectruHarmonics& h, int ifunc, double start, double step,
           double dstop, double lb, double ub, double& fmin, double& xmin,
           int& iconv);

// MINIMbis -- ansub2.f:1007. Same search as MINIM but over Fbis (the
// trend-discontinuity-patched function) and a 500-iteration outer cap
// (vs MINIM's 50) -- both faithfully preserved, not unified.
void minimbis(const SpectruHarmonics& h, int ifunc, double start, double step,
              double dstop, double lb, double ub, int haydif, int mq, int tol,
              double& fmin, double& xmin, int& iconv);

// GlobalMinim -- ansub2.f:1447. Sweeps `n_step+1` MINIMbis starting points
// evenly spaced over [lb,ub], keeping the best. iconv is MINIMbis's from the
// LAST sweep point evaluated (the oracle does not track per-point iconv).
void global_minim(const SpectruHarmonics& h, int ifunc, double lb, double ub,
                   int n_step, int haydif, int mq, int tol, double step,
                   double dstop, double& fmin, double& xmin, int& iconv);

// MinimGrid -- ansub2.f:1257. Brute-force grid search (step = pi/100000)
// over FUNC0, wcomp selects which component (1=seasonal, avoiding the mq
// seasonal-harmonic neighborhoods by +-epsphi degrees; 2=trend/cycle,
// skipping [0,epsphi); 3=transitory/cycle, full [0,pi]).
void minim_grid(const SpectruHarmonics& h, int ifunc, int mq, int epsphi,
                 int wcomp, double& fmin, double& xmin);

// ADDJ -- spectrum.f:3138. c = d1*a + d2*b (true-sign polynomial add,
// zero-padding the shorter operand). lplus1 = max(mplus1,nplus1).
void addj(const double* a, int mplus1, double d1, const double* b, int nplus1,
          double d2, double* c, int& lplus1);

// SPECTRU's outputs: the admissibility number qt1, the per-component
// spectrum-floor values, the isUgly/root-classification flags it can flip,
// and the harmonic-function COMMON state (Ut/Uc/V/Ff/Fh/...) later pieces
// of the canonical decomposition (DecompSpectrum, not yet ported) consume.
struct SpectruResult {
    SpectruHarmonics h;
    double qt1 = 0.0;
    double enot = 0.0, enoc = 0.0, estar = 0.0;
    bool is_ugly = false;
    int ncycth = 0;
    // Preliminary numerator harmonic functions after the qstar>pstar
    // correction (spectrum.f:967-973) -- ADDJ folds the quotient qt into Uc.
};

// SPECTRU -- spectrum.f:558-1190. thstar/qstar: model MA numerator
// (true-sign, thstar[0]=1). chi/nchi, cyc/ncyc, psi/npsi: the FULL
// (nonstationary*stationary) per-component AR denominators (chi=Chins*Chis
// etc, via CONV -- sigex.f:583-597, not yet a named helper; callers CONV
// them directly). pstar: length of Totden=Chi*Psi*Cyc. mq/bd/d: seasonal
// period / differencing orders. out/har: print-suppression flags (har==1
// tabular dumps are always skipped here, matching an oracle run with
// out!=0). root0c/rootpic/rootpis: F1RST's root-classification flags,
// consumed by the trend/cycle spectrum-minimum "isUgly" checks.
void spectru(const double* thstar, int qstar, const double* chi, int nchi,
             const double* cyc, int ncyc, const double* psi, int npsi,
             int pstar, int mq, int bd, int d, int out, int har, bool root0c,
             bool rootpic, bool rootpis, SpectruResult& r);

}  // namespace x13

#endif  // X13_SEATS_SPECTRU_HPP
