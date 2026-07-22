// regvar.hpp -- regression design-matrix builder (M2 regression-matrix chunk).
//
// One free function per ported Fortran routine; every routine that touches
// ex-COMMON state takes X13Context& ctx first. regvar.f fills the [X:y] matrix
// (mdldat.cmn Xy, row-major by observation: element (row,col) at
// Ncxy*(row-1)+col) from the parsed regression groups plus the possibly
// transformed data; the deterministic calendar kernels (td6var/td7var/addsef/
// adestr/estrmu) and the differencing integration (ratpos) are pre-model
// reproducible.
#ifndef X13_REGARIMA_REGVAR_HPP
#define X13_REGARIMA_REGVAR_HPP

#include <string>
#include "common/x13context.hpp"

namespace x13 {

// ratpos.f: power-series expansion of a rational polynomial 1/b(x) applied to
// the series in c (in place); b is the product of the operators
// [begopr..endopr] of (arimap, arimal, opr). Pointers are Fortran-base
// (arimap[0]==Arimap(1), opr[0]==Opr(0)).
void ratpos(int nelta, const double* arimap, const int* arimal, const int* opr,
            int begopr, int endopr, int neltc, double* c);

// ratneg.f: sibling of ratpos for a denominator in NEGATIVE powers (backward
// recursion, c[nelta-...] solved first). No neltc argument -- nelta bounds the
// series. Three-state flush per coefficient: |sum|>1e-300 stores sum;
// 0<|sum|<=1e-300 stores 0.0; sum exactly 0 leaves c[i] UNCHANGED (stale). No
// underflow guard on the accumulation (the oracle's under0 calls are disabled).
void ratneg(int nelta, const double* arimap, const int* arimal, const int* opr,
            int begopr, int endopr, double* c);

// copycl.f: copy column ifrmcl of from (nr x nfrmcl, row-major) into column
// itocl of to (nr x ntocl).
void copycl(const double* from, int nr, int nfrmcl, int ifrmcl, int ntocl,
            int itocl, double* to);

// gtrgpt.f: change-of-regime pointer vector (rgdtvc[0]==Rgdtvc(1)).
void gtrgpt(X13Context& ctx, const int* begdat, const int* rgdate, int rgzero,
            bool* rgdtvc, int nobs);

// td6var.f: the first six trading-day contrast variables (or the
// one-coefficient weekday variable) into Xy columns begcol..endcol.
void td6var(X13Context& ctx, const int* begdat, int isp, int numrxy, int numcxy,
            int begcol, int endcol, int smpday, double* xy, const bool* begrgm,
            bool td1c);

// td7var.f: the seventh trading-day variable (length-of-month/quarter, leap
// year, or stock length-of-month) into Xy column begcol.
void td7var(const int* begdat, int isp, int nrxy, int ncxy, int begcol, bool lom,
            bool ltdstk, bool mltadd, double* xy, const bool* begrgm);

// addsef.f: seasonal-effect contrast variables into Xy columns begcol..endcol.
void addsef(X13Context& ctx, const int* begdat, int numrxy, int numcxy,
            int begcol, int endcol, double* xy, const bool* begrgm);

// adsncs.f: trigonometric (sine-cosine) seasonal regressors into Xy columns
// begcol..endcol. Column titles are "sin(2pi*<h>t/<sp>)" / "cos(2pi*<h>t/<sp>)".
void adsncs(X13Context& ctx, const int* begdat, int numrxy, int numcxy,
            int begcol, int endcol, double* xy, const bool* begrgm);

// sceast.f: Statistics Canada Easter regressor value.
double sceast(int ndays, int pdays, bool first, bool ineast);

// adestr.f: Bell Easter holiday regressor into Xy column icol.
void adestr(const int* begdat, int nrxy, int ncxy, int isp, int icol, int ndays,
            int easidx, double* xy, bool xmeans, const double* emean, bool estock);

// estrmu.f: monthly (or quarterly) long-term means of the Easter regressor.
void estrmu(const int* begdat, int nrxy, int sp, int ndays, bool hlong,
            double* hmean, bool hstock);

// adlabr.f: Bell Labor-Day holiday regressor into Xy column icol (monthly only).
void adlabr(const int* begdat, int nrxy, int ncxy, int icol, int ndays,
            double* xy, bool xmeans);

// adthnk.f: Bell Thanksgiving-Christmas holiday regressor into Xy column icol.
void adthnk(const int* begdat, int nrxy, int ncxy, int icol, int ndays,
            double* xy, bool xmeans);

// regvar.f: build the [X:y] regression matrix (mdldat Xy) for the current
// model state. Outputs Nrxy (rows), Begxy (start date incl. backcasts), and
// Frstry (first element of Xy used in estimation).
void regvar(X13Context& ctx, const double* y, int nobpf, int fctdrp, int nfcst,
            int nbcst, const double* userx, const int* bgusrx, int nrusrx,
            int priadj, int reglom, int& nrxy, int* begxy, int& frstry,
            bool xmeans, bool elong);

}  // namespace x13

#endif  // X13_REGARIMA_REGVAR_HPP
