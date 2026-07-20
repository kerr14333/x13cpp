// seatsfact.hpp -- SEATS canonical-decomposition leaves: partial-fraction the
// pseudo-spectrum (PARFRA) and spectral-factor an autocovariance sequence into
// its canonical invertible MA (MAK1). Direct ports of the vendored Fortran in
// oracle/fortran/ansub2.f (PARFRA ansub2.f:1495, MAK1 ansub2.f:1615).
//
// These sit one tier above the pure poly/root leaves (seatspoly.hpp): MAK1
// drives RPQ (the C02AEF root finder) + CONJ, and both routines pull in a set of
// SEATS-private helpers (CONVM/CONJM/MLTSOL, SYMPOLY, ROOTC/SQROOTC, MPBC,
// grRoots/HalfRoots and their root-grouping subroutines) that live, unexposed,
// in the .cpp. Together they emit the component ARMA models that become the
// Step-1 (.mdc) corpus gate; NOT yet wired into any driver (SECOND/sigex).
//
// PARITY: MAK1's factorization is exact-equality-sensitive -- the invertible
// half-roots are selected by modulus comparisons and the polynomial is rebuilt
// from the C02AEF roots, so a last-ULP root difference changes theta/var. There
// is no explicit convergence loop; the tolerance gating is entirely in RPQ's
// Newton iteration and the modulus/argument classification. Ported verbatim.
//
// Array-index convention matches roots.cpp: Fortran 1-based coefficient vectors
// are passed as ordinary 0-based C pointers (A(1..k) -> A[0..k-1]); the .cpp
// bridges to 1-based internal buffers where the transcription needs it.
#ifndef X13_SEATS_SEATSFACT_HPP
#define X13_SEATS_SEATSFACT_HPP

namespace x13 {

// PARFRA: partial-fraction RT(x)/(T(x)S(x)) = U(x)/T(x) + V(x)/S(x), where x =
// cos(w) and RT,T,S are harmonic functions (polynomials in powers of cos w).
// Solves the linear system for the numerators U (length nu = nt-1) and V
// (length nv = ns-1). rt must have at least (nt-1)+(ns-1) coefficients.
// oracle/fortran/ansub2.f:1495.
void parfra(const double* rt, int nrt, const double* t, int nt, const double* s,
            int ns, double* u, int& nu, double* v, int& nv);

// MAK1: compute the canonical invertible MA process whose autocovariance
// function is ufin. The input packs the (doubled) autocovariances:
//   ufin(1) = gamma(0),  ufin(k) = 2*gamma(k-1)  for k = 2..nufin.
// On return theta(1..ntheta) is the invertible MA polynomial (theta(1)=1) and
// var its innovation variance; toterr is the total squared mismatch between the
// factored ACF and ufin. nnio!=0 nudges any exact unit root inward using xl
// (the SEATS /unitmak/ XL option); nnio==0 leaves roots untouched. Printing
// (caption/Nio) and the Lfatal abort path are deferred. oracle/fortran/ansub2.f:1615.
void mak1(const double* ufin, int nufin, double* theta, int& ntheta,
          double& var, int nnio, double xl, double& toterr);

}  // namespace x13

#endif  // X13_SEATS_SEATSFACT_HPP
