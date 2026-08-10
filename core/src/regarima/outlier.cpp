// outlier.cpp -- automatic outlier identification leaves (see outlier.hpp).
// Faithful ports of makotl.f and ttest.f; 1-based Fortran index arithmetic is
// preserved with -1 offsets. Type-indexed arrays (ltest/ltstpt/propt/snglr/
// mxcol) follow the caller's contiguous [AO,LS,TC] slice convention: element
// (type) lives at index type-1 (prm::AO-1 = 0, prm::LS-1 = 1, prm::TC-1 = 2).
#include "regarima/outlier.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "numeric/numeric.hpp"      // ddot, dpmpar, dppdi, yprmy, dpeq, medabs
#include "specparse/specparse.hpp"  // strinx, ctodat, dfdate, addate, inpter,
                                    // adrgef, dlrgef, eltlen, getstr, copy, setint
#include "regarima/armafl.hpp"      // armafl
#include "regarima/estimate.hpp"    // rgarma
#include "x11/x11reg.hpp"        // regx11, prterx_if_singular (idotlr.f:878/995)
#include "gen/model.hpp"            // prm::PB, POTLR, AO, LS, TC, RP, TLS, ...
#include "gen/notset.hpp"           // prm::NOTSET
#include "gen/srslen.hpp"           // prm::PLEN

namespace x13 {

namespace {
const char* const CMO[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                             "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

constexpr double PI_ = 3.14159265358979;

// ppnd.f (AS 111) -- normal deviate for lower-tail area p; ier=1 on p<=0/>=1.
double ppnd(double p, int& ier) {
    const double SPLIT = 0.42;
    const double A0 = 2.50662823884, A1 = -18.61500062529, A2 = 41.39119773534,
                 A3 = -25.44106049637;
    const double B1 = -8.47351093090, B2 = 23.08336743743, B3 = -21.06224101826,
                 B4 = 3.13082909833;
    const double C0 = -2.78718931138, C1 = -2.29796479134, C2 = 4.85014127135,
                 C3 = 2.32121276858;
    const double D1 = 3.54388924762, D2 = 1.63706781897;
    ier = 0;
    double q = p - 0.5;
    if (std::fabs(q) <= SPLIT) {
        double r = q * q;
        return q * (((A3 * r + A2) * r + A1) * r + A0) /
               ((((B4 * r + B3) * r + B2) * r + B1) * r + 1.0);
    }
    double r = p;
    if (q > 0.0) r = 1.0 - p;
    if (r <= 0.0) {
        ier = 1;
        return 0.0;
    }
    r = std::sqrt(-std::log(r));
    double v = (((C3 * r + C2) * r + C1) * r + C0) / ((D2 * r + D1) * r + 1.0);
    if (q < 0.0) v = -v;
    return v;
}

// lassol.f -- solve AX=B (n<=3) by Gaussian elimination with partial pivoting
// and row equilibration. a is column-major with leading dimension m. iflag=1 on
// success, 2 if singular. The oracle's EQUIVALENCEd scalars are used in disjoint
// scopes, so distinct locals are exact.
void lassol(int n, const double* a, const double* b, int m, double* x,
            int& iflag) {
    const int np1 = n + 1;
    double ab[3][4];
    auto A = [&](int i, int j) { return a[(j - 1) * m + (i - 1)]; };
    for (int i = 1; i <= n; ++i) {
        double rowmax = 0.0;
        for (int j = 1; j <= n; ++j) rowmax = std::max(rowmax, std::fabs(A(i, j)));
        double scale = 1.0 / rowmax;
        for (int j = 1; j <= n; ++j) ab[i - 1][j - 1] = A(i, j) * scale;
        ab[i - 1][np1 - 1] = b[i - 1] * scale;
    }
    for (int k = 1; k <= n - 1; ++k) {
        double big = 0.0;
        int idxpiv = k;
        for (int i = k; i <= n; ++i) {
            double t = std::fabs(ab[i - 1][k - 1]);
            if (big < t) {
                big = t;
                idxpiv = i;
            }
        }
        if (dpeq(big, 0.0)) {
            iflag = 2;
            return;
        }
        if (idxpiv != k)
            for (int i = k; i <= np1; ++i)
                std::swap(ab[k - 1][i - 1], ab[idxpiv - 1][i - 1]);
        for (int i = k + 1; i <= n; ++i) {
            double quot = ab[i - 1][k - 1] / ab[k - 1][k - 1];
            for (int j = k + 1; j <= np1; ++j)
                ab[i - 1][j - 1] -= quot * ab[k - 1][j - 1];
        }
    }
    if (!dpeq(ab[n - 1][n - 1], 0.0)) {
        x[n - 1] = ab[n - 1][np1 - 1] / ab[n - 1][n - 1];
        for (int ib = 2; ib <= n; ++ib) {
            int i = np1 - ib;
            double sum = 0.0;
            for (int j = i + 1; j <= n; ++j) sum += ab[i - 1][j - 1] * x[j - 1];
            x[i - 1] = (ab[i - 1][np1 - 1] - sum) / ab[i - 1][i - 1];
        }
        iflag = 1;
        return;
    }
    iflag = 2;
}
}  // namespace

// setcvl.f -- large-sample (Ljung) critical value approximation. Namespace
// scope, not file-local: editor.f:1752 calls it directly when Cvxtyp is set.
double setcvl(int nspobs, double cvalfa) {
    if (nspobs == 1) return prm::DNOTST;   // undefined for a 1-point span
    double pmod = 2.0 - std::sqrt(1.0 + cvalfa);
    double dnobs = nspobs;
    double acv = std::sqrt(2.0 * std::log(dnobs));
    double bcv = acv - (std::log(std::log(dnobs)) + std::log(4.0 * PI_)) /
                           (2.0 * acv);
    double xcv = -std::log(-0.5 * std::log(pmod));
    return (xcv / acv) + bcv;
}

// setcv.f -- default outlier critical value from the test-span length.
double setcv(int nspobs, double cvalfa) {
    const double x[3] = {2.0, 100.0, 200.0};
    if (nspobs == 1) {
        int iflag = 0;
        double v = ppnd(1.0 - (cvalfa / 2.0), iflag);
        return (iflag == 1) ? prm::DNOTST : v;
    }
    double dnobs = nspobs;
    double xmat[9], y[3], beta[3];
    for (int i = 1; i <= 3; ++i) {
        if (i == 1) {
            int iflag = 0;
            y[0] = ppnd((1.0 + std::sqrt(1.0 - cvalfa)) / 2.0, iflag);
            if (iflag == 1) return prm::DNOTST;
        } else {
            y[i - 1] = setcvl(static_cast<int>(x[i - 1]), cvalfa);
        }
        double xi = x[i - 1];
        double col3 = std::sqrt(2.0 * std::log(xi));
        // xmat column-major (i,j) -> xmat[(j-1)*3 + (i-1)].
        xmat[0 * 3 + (i - 1)] = 1.0;                                    // (i,1)
        xmat[2 * 3 + (i - 1)] = col3;                                   // (i,3)
        xmat[1 * 3 + (i - 1)] =
            (std::log(std::log(xi)) + std::log(4.0 * PI_)) / (2.0 * col3);  // (i,2)
    }
    int iflag = 0;
    lassol(3, xmat, y, 3, beta, iflag);
    if (iflag == 2) return prm::DNOTST;
    double acv = std::sqrt(2.0 * std::log(dnobs));
    double bcv = (std::log(std::log(dnobs)) + std::log(4.0 * PI_)) / (2.0 * acv);
    return beta[0] + beta[1] * bcv + beta[2] * acv;
}

// wrtdat.f -- date to "yyyy[.mon]" per seasonal period.
std::string wrtdat(const int* idate, int sp) {
    std::string s = std::to_string(idate[prm::YR - 1]);
    if (sp > 1) {
        s += ".";
        if (sp == 12)
            s += CMO[idate[prm::MO - 1] - 1];
        else
            s += std::to_string(idate[prm::MO - 1]);
    }
    return s;
}

// wrtotl.f -- outlier title = 2-char type + date (+ ramp end for itype 5).
std::string wrtotl(int itype, int begotl, int endotl, const int* begdat,
                   int sp) {
    static const char* const OTLTYP[6] = {"AO", "LS", "TC", "SO", "Rp", "MV"};
    int otldat[2];
    addate(begdat, sp, begotl - 1, otldat);
    std::string s = wrtdat(otldat, sp);
    if (itype == 5) {  // NOTYPE-1: ramp
        s += "-";
        int otldat2[2];
        addate(begdat, sp, endotl - 1, otldat2);
        s += wrtdat(otldat2, sp);
    }
    return std::string(OTLTYP[itype - 1]) + s;
}

// rdotlr.f -- parse an outlier title into type + time index(es).
void rdotlr(X13Context& ctx, const std::string& otlttl, const int* begspn,
            int sp, int& otlind, int& begotl, int& endotl, bool& locok) {
    // OTLDIC 2-char tokens: ao ls tc rp mv tl so qi qd (indices 1..9).
    static const char OTLDIC[] = "aolstcrpmvtlsoqiqd";
    static const int otlptr[10] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19};
    constexpr int PEXERR = 3;
    int dummy[2] = {0, 0};

    endotl = 0;
    locok = true;
    otlind = strinx(false, OTLDIC, otlptr, 1, 9, otlttl.substr(0, 2));
    if (otlind == 0) {
        inpter(ctx, PEXERR, dummy,
               "Outlier type, \"" + otlttl +
                   "\" is not an AO, LS, RP, SO, TL, TC, MV, QI or QD.");
        locok = false;
        return;
    }
    int ipos = 3;  // 1-based char position after the 2-char type
    int begdat[2];
    ctodat(otlttl, sp, ipos, begdat, locok);
    if (!locok) {
        inpter(ctx, PEXERR, dummy,
               "Outlier \"" + otlttl + "\" does not occur on a valid date.");
        return;
    }
    dfdate(begdat, begspn, sp, begotl);
    begotl = begotl + 1;

    // Ramp / temporary-level-shift / quadratic-ramp end date.
    if (locok && (otlind == prm::RP || otlind == prm::TLS || otlind == 8 ||
                  otlind == 9)) {
        if (otlttl[ipos - 1] != '-') {
            inpter(ctx, PEXERR, dummy,
                   "\"" + otlttl + "\" is an invalid ramp outlier.");
            locok = false;
        } else {
            ipos = ipos + 1;
            int enddat[2];
            ctodat(otlttl, sp, ipos, enddat, locok);
            if (!locok) {
                inpter(ctx, PEXERR, dummy,
                       "Ramp outlier \"" + otlttl +
                           "\" does not have a valid end date.");
            } else {
                dfdate(enddat, begspn, sp, endotl);
                endotl = endotl + 1;
            }
        }
    }
}

// makotl.f -- build the AO/LS/TC regressor(s) for time t0. The active types are
// packed interleaved into otlvar with stride notlr; dsp gives each type's offset
// back from the block end. TC decays geometrically by tcalfa after t0.
void makotl(int t0, int nr, const int* ltest, double* otlvar, int& notlr,
            double tcalfa, int /*sp*/) {
    constexpr double ONE = 1.0, MONE = -1.0, ZERO = 0.0;
    const int AO = prm::AO, LS = prm::LS, TC = prm::TC;
    // dsp(AO)=dsp[AO-1], dsp(LS)=dsp[LS-1); length POTLR-1 (no TC slot).
    int dsp[prm::POTLR - 1] = {0, 0};
    if (ltest[TC - 1] == 1) {
        dsp[AO - 1]++;
        dsp[LS - 1]++;
    }
    if (ltest[LS - 1] == 1) dsp[AO - 1]++;
    notlr = dsp[AO - 1];
    if (ltest[AO - 1] == 1) notlr++;

    // Observations before t0.
    for (int i = notlr; i <= notlr * (t0 - 1); i += notlr) {
        if (ltest[AO - 1] == 1) otlvar[i - dsp[AO - 1] - 1] = ZERO;
        if (ltest[LS - 1] == 1) otlvar[i - dsp[LS - 1] - 1] = MONE;
        if (ltest[TC - 1] == 1) otlvar[i - 1] = ZERO;
    }
    // Observation t0.
    int i = t0 * notlr;
    if (ltest[AO - 1] == 1) otlvar[i - dsp[AO - 1] - 1] = ONE;
    if (ltest[LS - 1] == 1) otlvar[i - dsp[LS - 1] - 1] = ZERO;
    if (ltest[TC - 1] == 1) otlvar[i - 1] = ONE;
    // Observations after t0.
    for (i = i + notlr; i <= nr * notlr; i += notlr) {
        if (ltest[AO - 1] == 1) otlvar[i - dsp[AO - 1] - 1] = ZERO;
        if (ltest[LS - 1] == 1) otlvar[i - dsp[LS - 1] - 1] = ZERO;
        if (ltest[TC - 1] == 1) otlvar[i - 1] = otlvar[i - notlr - 1] * tcalfa;
    }
}

// ttest.f -- proportional outlier t-statistics via an augmented Cholesky update
// of the estimated regression. For each active type it forms o'o, X'o and y'o
// against the filtered design, solves L*l = X'o, and returns b/se(b)*mse =
// (o'y - l'w)/sqrt(o'o - l'l).
void ttest(const double* xy, int nspobs, int ncxy, const double* chlxpx,
           const double* otlvar, const int* ltstpt, int* mxcol, double* propt,
           bool* snglr) {
    constexpr double ZERO = 0.0;
    const int POTLR = prm::POTLR;
    for (int t = 0; t < POTLR; ++t) snglr[t] = false;

    const int nb = ncxy - 1;
    const int neltxy = nspobs * ncxy;
    const int nxpx = nb * ncxy / 2;
    const double xl = std::sqrt(dpmpar(2));

    // Active types (otype[k] holds the 1-based type value).
    int otype[prm::POTLR];
    int notlr = 0;
    for (int i = 1; i <= POTLR; ++i)
        if (ltstpt[i - 1] == 1) otype[notlr++] = i;

    const int lstride = ncxy + 1;  // l[k][j], j in 1..ncxy
    std::vector<double> oomll(notlr, 0.0), oymlw(notlr, 0.0), tmp(notlr, 0.0);
    std::vector<double> l(static_cast<std::size_t>(notlr) * lstride, 0.0);
    std::vector<double> l1(lstride, 0.0);

    // o'o per type (underflow-skipping), interleaved cycle over notlr.
    int i2 = 0;
    for (int ielt = 1; ielt <= notlr * nspobs; ++ielt) {
        if (std::fabs(otlvar[ielt - 1]) > xl)
            oomll[i2] += otlvar[ielt - 1] * otlvar[ielt - 1];
        if (++i2 >= notlr) i2 = 0;
    }

    // [X:y]'o : column j dotted (strided by ncxy) against the interleaved otlvar.
    for (int j = 1; j <= ncxy; ++j) {
        for (int k = 0; k < notlr; ++k) tmp[k] = 0.0;
        int i = 1;
        for (int ielt = j; ielt <= neltxy; ielt += ncxy) {
            for (int k = 0; k < notlr; ++k) {
                tmp[k] += xy[ielt - 1] * otlvar[i - 1];
                ++i;
            }
        }
        for (int k = 0; k < notlr; ++k) l[k * lstride + j] = tmp[k];
    }

    // Solve L*l = X'o, accumulating o'y - l'w and o'o - l'l.
    for (int k = 0; k < notlr; ++k) oymlw[k] = l[k * lstride + ncxy];
    int ielt = 0;
    for (int i = 1; i <= nb; ++i) {
        for (int k = 0; k < notlr; ++k) {
            for (int j = 1; j <= ncxy; ++j) l1[j] = l[k * lstride + j];
            tmp[k] = l[k * lstride + i] - ddot(i - 1, &chlxpx[ielt], 1, &l1[1], 1);
        }
        ielt += i;
        for (int k = 0; k < notlr; ++k) {
            tmp[k] = tmp[k] / chlxpx[ielt - 1];
            l[k * lstride + i] = tmp[k];
            oymlw[k] -= chlxpx[nxpx + i - 1] * tmp[k];
            oomll[k] -= tmp[k] * tmp[k];
        }
    }

    // b/se(b)*mse per type; flag singular (o'o - l'l <= 0).
    for (int k = 0; k < notlr; ++k) {
        if (oomll[k] <= ZERO) {
            snglr[otype[k] - 1] = true;
            propt[otype[k] - 1] = ZERO;
        } else {
            propt[otype[k] - 1] = oymlw[k] / std::sqrt(oomll[k]);
        }
    }

    // Rank the types by |t|, largest first (insertion sort).
    mxcol[0] = otype[0];
    if (notlr > 1) {
        std::vector<double> tv(notlr);
        tv[0] = propt[otype[0] - 1];
        for (int i = 2; i <= notlr; ++i) {
            tv[i - 1] = propt[otype[i - 1] - 1];
            mxcol[i - 1] = otype[i - 1];
            int j = i - 1;
            bool lgo = true;
            while (lgo && j > 0) {
                if (std::fabs(tv[j - 1]) < std::fabs(tv[j])) {
                    tv[j] = tv[j - 1];
                    mxcol[j] = mxcol[j - 1];
                    tv[j - 1] = propt[otype[i - 1] - 1];
                    mxcol[j - 1] = otype[i - 1];
                } else {
                    lgo = false;
                }
                --j;
            }
        }
    }
}

// deltst.f -- backward-deletion t-statistics for the auto-outlier columns.
void deltst(X13Context& ctx, int nefobs, int begcol, int endcol, double* mint,
            int* mini, int* minptr, bool /*lauto*/, bool /*lxreg*/) {
    constexpr double ZERO = 0.0;
    const int POTLR = prm::POTLR, NOTSET = prm::NOTSET;
    model_cmn& M = ctx.model;
    mdldat_cmn& D = ctx.mdldat;
    const int nb = M.nb, ncxy = M.ncxy;

    // Residual rmse = last diagonal of chol([X:y]'[X:y]) / sqrt(nefobs).
    const int nelt = nb * ncxy / 2;
    double rmse = D.chlxpx(nelt + ncxy);
    if (dpeq(rmse, ZERO)) {   // residual rmse zero -> cannot test (abend)
        ctx.error.lfatal = true;
        return;
    }
    rmse = rmse / std::sqrt(static_cast<double>(nefobs));

    // (X'X)^-1 from the packed Cholesky of the leading Nb x Nb block.
    std::vector<double> xpxinv(nelt > 0 ? nelt : 1);
    copy(D.chlxpx.data(), nelt, 1, xpxinv.data());
    double det[2] = {0, 0};
    dppdi(xpxinv.data(), nb, det, 1);
    setint(NOTSET, POTLR, mini);

    int ielt = 0;
    for (int i = begcol; i <= endcol; ++i) {
        std::string tmpttl;
        int ntmpcr = 0;
        getstr(ctx, M.colttl.data(), M.colptr.data(), M.ncoltl, i, tmpttl,
               ntmpcr);
        int otltyp = 0, t0 = 0, itmp = 0;
        bool locok = true;
        if (!ctx.error.lfatal)
            rdotlr(ctx, tmpttl, D.begspn.data(), M.sp, otltyp, t0, itmp, locok);
        if (ctx.error.lfatal) return;
        // Diagonal packed index of column i in (X'X)^-1.
        if (i == begcol)
            ielt = begcol * (begcol + 1) / 2;
        else
            ielt += i;
        double tval = D.b(i) / std::sqrt(xpxinv[ielt - 1]) / rmse;
        if (mini[otltyp - 1] == NOTSET) {
            mini[otltyp - 1] = i;
            mint[otltyp - 1] = tval;
        } else if (std::fabs(tval) <= std::fabs(mint[otltyp - 1])) {
            mini[otltyp - 1] = i;
            mint[otltyp - 1] = tval;
        }
    }

    // Rank the per-type minima by |t| ascending (minptr[0] = smallest).
    setint(NOTSET, POTLR, minptr);
    int i2 = 1;
    for (int i = 1; i <= POTLR; ++i) {
        if (mini[i - 1] != NOTSET) {
            minptr[i2 - 1] = i;
            if (i2 > 1) {
                int j = i2 - 1;
                while (j > 0) {
                    if (std::fabs(mint[minptr[j - 1] - 1]) >
                        std::fabs(mint[minptr[j] - 1])) {
                        minptr[j] = minptr[j - 1];
                        minptr[j - 1] = i;
                    }
                    --j;
                }
            }
            ++i2;
        }
    }
}

// addotl.f -- reconstruct outlier regressor columns from their titles. Out-of-
// span columns are dropped via dlrgef (the diagnostic NOTE prints are deferred
// to the .out milestone). Row-major Xy with leading dimension ncxy.
void addotl(X13Context& ctx, const int* bgdtxy, int nrxy, int iymx, int begcol,
            int& endcol) {
    constexpr double ZERO = 0.0, ONE = 1.0, MONE = -1.0;
    using namespace prm;
    model_cmn& M = ctx.model;
    mdldat_cmn& D = ctx.mdldat;
    double* xy = D.xy.data();
    const int ncxy = M.ncxy;
    const int nspobs = D.nspobs;
    const int sp = M.sp;
    const double tcalfa = M.tcalfa;

    if (begcol < 1 || endcol > ncxy - 1 || endcol < begcol) {
        ctx.error.lfatal = true;   // invalid column range (abend)
        return;
    }

    int icol = begcol;
    while (icol <= endcol) {
        std::string str;
        int nchr = 0;
        getstr(ctx, M.colttl.data(), M.colptr.data(), M.ncoltl, icol, str, nchr);
        if (ctx.error.lfatal) return;
        int otltyp = 0, begotl = 0, endotl = 0;
        bool locok = true;
        rdotlr(ctx, str, bgdtxy, sp, otltyp, begotl, endotl, locok);
        if (!locok) ctx.error.lfatal = true;
        if (ctx.error.lfatal) return;

        // XY(irow) == Xy(ncxy*(irow-1)+icol) 1-based -> 0-based.
        auto XY = [&](int irow) -> double& { return xy[ncxy * (irow - 1) + icol - 1]; };
        // Drop the current column and shrink the range (column NOTE deferred).
        auto drop = [&]() {
            dlrgef(ctx, icol, nrxy, 1);
            --endcol;
        };
        const double drmp = static_cast<double>(endotl - begotl);

        if (otltyp == AO || otltyp == MV) {
            if (begotl > iymx + nspobs || begotl < iymx + 1) {
                drop();
            } else {
                for (int irow = 1; irow <= nrxy; ++irow) XY(irow) = ZERO;
                XY(begotl) = ONE;
                ++icol;
            }
        } else if (otltyp == LS) {
            if (begotl > iymx + nspobs || begotl < iymx + 2) {
                drop();
            } else {
                for (int irow = 1; irow <= begotl - 1; ++irow) XY(irow) = MONE;
                for (int irow = begotl; irow <= nrxy; ++irow) XY(irow) = ZERO;
                ++icol;
            }
        } else if (otltyp == RP) {
            if (endotl <= begotl || begotl >= iymx + nspobs ||
                endotl <= iymx + 1 ||
                (begotl <= iymx + 1 && endotl >= iymx + nspobs)) {
                drop();
            } else {
                for (int irow = 1; irow <= begotl; ++irow) XY(irow) = -drmp;
                for (int irow = std::max(1, begotl + 1);
                     irow <= std::min(endotl - 1, nrxy); ++irow)
                    XY(irow) = (irow - std::max(1, begotl + 1) + 1) - drmp;
                for (int irow = endotl; irow <= nrxy; ++irow) XY(irow) = ZERO;
                ++icol;
            }
        } else if (otltyp == TLS) {
            if (endotl <= begotl || begotl >= iymx + nspobs ||
                endotl <= iymx + 1 ||
                (begotl <= iymx + 1 && endotl >= iymx + nspobs)) {
                drop();
            } else {
                if ((begotl - 1) > 1)
                    for (int irow = 1; irow <= begotl - 1; ++irow) XY(irow) = ZERO;
                for (int irow = std::max(1, begotl); irow <= std::min(endotl, nrxy);
                     ++irow)
                    XY(irow) = ONE;
                if ((endotl + 1) < nrxy)
                    for (int irow = endotl + 1; irow <= nrxy; ++irow) XY(irow) = ZERO;
                ++icol;
            }
        } else if (otltyp == TC) {
            if (begotl > iymx + nspobs || begotl < iymx + 1) {
                drop();
            } else {
                for (int irow = 1; irow <= begotl - 1; ++irow) XY(irow) = ZERO;
                XY(begotl) = ONE;
                for (int irow = begotl + 1; irow <= nrxy; ++irow)
                    XY(irow) = XY(irow - 1) * tcalfa;
                ++icol;
            }
        } else if (otltyp == SO) {
            if (begotl > iymx + nspobs || begotl < iymx + 1) {
                drop();
            } else {
                for (int irow = begotl; irow <= nrxy; ++irow) XY(irow) = ZERO;
                int imod = begotl % sp;
                double drmp2 = ONE / static_cast<double>(sp - 1);
                for (int irow = begotl - 1; irow >= 1; --irow)
                    XY(irow) = (irow % sp == imod) ? MONE : drmp2;
                ++icol;
            }
        } else if (otltyp == QI) {
            if (endotl <= begotl || begotl >= iymx + nspobs ||
                endotl <= iymx + 1 ||
                (begotl <= iymx + 1 && endotl >= iymx + nspobs)) {
                drop();
            } else {
                for (int irow = 1; irow <= begotl; ++irow) XY(irow) = -(drmp * drmp);
                for (int irow = std::max(1, begotl + 1);
                     irow <= std::min(endotl - 1, nrxy); ++irow) {
                    double drow = irow - std::max(1, begotl + 1) + 1;
                    XY(irow) = (drow * drow) - (drmp * drmp);
                }
                for (int irow = endotl; irow <= nrxy; ++irow) XY(irow) = ZERO;
                ++icol;
            }
        } else if (otltyp == QD) {
            if (endotl <= begotl || begotl >= iymx + nspobs ||
                endotl <= iymx + 1 ||
                (begotl <= iymx + 1 && endotl >= iymx + nspobs)) {
                drop();
            } else {
                for (int irow = 1; irow <= begotl; ++irow) XY(irow) = -(drmp * drmp);
                for (int irow = std::max(1, begotl + 1);
                     irow <= std::min(endotl - 1, nrxy); ++irow) {
                    double drow = irow - std::max(1, begotl + 1) + 1;
                    XY(irow) = -((drmp - drow) * (drmp - drow));
                }
                for (int irow = endotl; irow <= nrxy; ++irow) XY(irow) = ZERO;
                ++icol;
            }
        } else {
            ctx.error.lfatal = true;   // not an outlier type (abend)
            return;
        }
        if (ctx.error.lfatal) return;
    }
}

// coladd.f -- make room for naddc columns at begcol by shifting the trailing
// columns right, walking rows from the last backward so the in-place moves do
// not clobber unread data. Row-major xy with leading dimension ncxy; 1-based
// Fortran index arithmetic preserved (-1 on the C++ accesses).
void coladd(int begcol, int endcol, int nrxy, int /*peltxy*/, double* xy,
            int& ncxy) {
    int naddc = endcol - begcol + 1;
    int nnewc = ncxy + naddc;
    int offset = nrxy * naddc;
    int iend = nrxy * ncxy;
    int ibeg = iend - ncxy + begcol;
    for (int j = iend; j >= ibeg; --j) xy[j + offset - 1] = xy[j - 1];
    for (int i = nrxy - 1; i >= 1; --i) {
        offset = i * naddc;
        iend = ibeg - 1;
        ibeg = iend - ncxy + 1;
        for (int j = iend; j >= ibeg; --j) xy[j + offset - 1] = xy[j - 1];
    }
    ncxy = nnewc;
}

// idotlr.f -- automatic outlier identification driver (forward addition +
// backward deletion). Printing, save files, the x11-regression path, and the
// diagnostic "almost outlier" re-scan are omitted; the numeric identification
// and the re-estimated model match the oracle.
// x11reg.cpp: OLS re-fit for the Lxreg outlier path (forward-declared to keep the
// regarima -> x11 dependency out of the header).
bool regx11(X13Context& ctx, double* aout, int* naout, int* nefout);

void idotlr(X13Context& ctx, bool ltstao, bool ltstls, bool ltsttc, bool ladd1,
            const double* critvl, double /*cvrduc*/, const int* begtst,
            const int* endtst, int& nefobs, bool lestim, int mxiter, int mxnlit,
            bool lauto, double* a, bool lxreg) {
    using namespace prm;
    constexpr double ZERO = 0.0;
    constexpr int PA = PLEN + 2 * PORDER;
    constexpr int PXA = PA * (PB + 1);
    constexpr int POA = PA * POTLR;
    constexpr int PXY = PLEN * (PB + 1);
    const char* AOTLTL = "Automatically Identified Outliers";
    const int mxcode[POTLR] = {PRGTAA, PRGTAL, PRGTAT};

    model_cmn& M = ctx.model;
    mdldat_cmn& D = ctx.mdldat;
    const int nspobs = D.nspobs;
    const int sp = M.sp;
    bool lautmp = lauto;

    // tstpt[(t0-1)*POTLR + (type-1)] : the [AO,LS,TC] test flags per t0, laid out
    // so the 3 flags for one t0 are contiguous (makotl/ttest read that slice).
    std::vector<int> tstpt(static_cast<std::size_t>(POTLR) * (nspobs + 1), 0);
    auto TP = [&](int type, int t0) -> int& {
        return tstpt[static_cast<std::size_t>(t0 - 1) * POTLR + (type - 1)];
    };
    auto slice = [&](int t0) -> int* {
        return &tstpt[static_cast<std::size_t>(t0 - 1) * POTLR];
    };

    // Current auto-outlier group + its size.
    auto find_otlgrp = [&]() {
        return strinx(false, M.grpttl.data(), M.grpptr.data(), 1, M.ngrptl,
                      AOTLTL);
    };
    int otlgrp = find_otlgrp();
    int oldotl = 0;
    if (otlgrp > 0) {
        eltlen(ctx, otlgrp, M.grp.data(), M.ngrp, oldotl);
        if (ctx.error.lfatal) return;
    }

    // Test-point span (relative to the model span).
    int itmp = 0;
    dfdate(begtst, D.begspn.data(), sp, itmp);
    int ibgtst = std::max(itmp + 1, 1);
    dfdate(endtst, D.begspn.data(), sp, itmp);
    int iedtst = std::min(itmp + 1, nspobs);

    // Enable the requested test points, then remove the degenerate LS/TC edges.
    for (int t0 = ibgtst; t0 <= iedtst; ++t0) {
        if (ltstao) TP(AO, t0) = 1;
        if (ltstls) TP(LS, t0) = 1;
        if (ltsttc) TP(TC, t0) = 1;
    }
    if (ltstls) {
        TP(LS, 1) = 0;
        if (ltstao) {
            TP(LS, 2) = 0;
            TP(LS, nspobs) = 0;
        }
    }
    if (ltsttc && ltstao) TP(TC, nspobs) = 0;

    // Exclude user-specified and (if fixed) pre-set outlier columns from testing.
    auto exclude_col = [&](const char* ttl, const int* ptr, int nttl, int icol) {
        std::string tmpttl;
        int ntmpcr = 0;
        getstr(ctx, ttl, ptr, nttl, icol, tmpttl, ntmpcr);
        if (ctx.error.lfatal) return;
        int otltyp = 0, t0 = 0, ie = 0;
        bool locok = true;
        rdotlr(ctx, tmpttl, D.begspn.data(), sp, otltyp, t0, ie, locok);
        if (ctx.error.lfatal) return;
        if (otltyp == AO || otltyp == MV) TP(AO, t0) = 0;
        if (otltyp == LS || otltyp == MV) TP(LS, t0) = 0;
        if (otltyp == TC || otltyp == MV) TP(TC, t0) = 0;
    };
    for (int icol = 1; icol <= M.ncxy - 1; ++icol) {
        int rt = M.rgvrtp(icol);
        if (rt == PRGTAO || rt == PRGTLS || rt == PRGTTC || rt == PRGTSO ||
            rt == PRSQAO || rt == PRSQLS || rt == PRGTMV || rt == PRGTAA ||
            rt == PRGTAL || rt == PRGTAT) {
            exclude_col(M.colttl.data(), M.colptr.data(), M.ncoltl, icol);
            if (ctx.error.lfatal) return;
        }
    }
    if (M.iregfx >= 2) {
        for (int icol = 1; icol <= ctx.fxreg.nfxttl; ++icol) {
            int ft = ctx.fxreg.fxtype(icol);
            if (ft == PRGTAO || ft == PRGTLS || ft == PRGTTC || ft == PRGTMV ||
                ft == PRGTAA || ft == PRGTAL || ft == PRGTAT || ft == PRSQAO ||
                ft == PRSQLS) {
                exclude_col(ctx.fxreg.cfxttl.data(), ctx.fxreg.cfxptr.data(),
                            ctx.fxreg.nfxttl, icol);
                if (ctx.error.lfatal) return;
            }
        }
    }

    // Work vectors.
    std::vector<double> txa(PXA), otlvar(POA);
    double tvalt0[POTLR], critt[POTLR];
    int mxtype[POTLR];
    bool singlr[POTLR];

    // ---- Forward addition ----------------------------------------------------
    while (true) {
        std::string mxotl;
        double mxotlb = 0.0, mxotlt = 0.0;
        int mxott0 = 0, mxottp = 0;

        copy(D.xy.data(), nspobs * M.ncxy, 1, txa.data());
        int na = 0, info = 0;
        if (lxreg) {
            // idotlr.f:340-345 -- pure OLS, use the design copy unfiltered.
            na = nspobs;
        } else {
            armafl(ctx, nspobs, M.ncxy, false, false, txa.data(), na, PXA, info);
            if (info > 0) {
                ctx.error.lfatal = true;
                return;
            }
            if (ctx.error.lfatal) return;
        }

        // Robust and normal root mean square error of the residuals.
        double rbmse = 0.0;
        if ((M.lar || M.lma) && !lxreg)
            medabs(&a[M.mxmalg], nefobs, rbmse);
        else
            medabs(&a[0], nefobs, rbmse);
        if (ctx.error.lfatal) return;
        rbmse = rbmse / 0.6745;
        double rmse = 0.0;
        yprmy(a, na, rmse);
        rmse = std::sqrt(rmse / nefobs);
        if (dpeq(rbmse, ZERO)) {
            ctx.error.lfatal = true;
            return;
        }

        critt[AO - 1] = critvl[AO - 1] * rbmse;
        critt[LS - 1] = critvl[LS - 1] * rbmse;
        critt[TC - 1] = critvl[TC - 1] * rbmse;
        int oldnc = M.ncxy;

        // Scan every test point.
        for (int t0 = ibgtst; t0 <= iedtst; ++t0) {
            if (TP(AO, t0) != 1 && TP(LS, t0) != 1 && TP(TC, t0) != 1) continue;
            int ntype = 0;
            makotl(t0, nspobs, slice(t0), otlvar.data(), ntype, M.tcalfa, sp);
            if (lxreg) {
                na = nspobs;   // idotlr.f:415-420 -- OLS path, no ARMA filter.
            } else {
                armafl(ctx, nspobs, ntype, false, false, otlvar.data(), na, POA,
                       info);
                if (info > 0) {
                    ctx.error.lfatal = true;
                    return;
                }
            }
            setint(NOTSET, POTLR, mxtype);
            ttest(txa.data(), na, oldnc, D.chlxpx.data(), otlvar.data(),
                  slice(t0), mxtype, tvalt0, singlr);
            // Drop singular test points.
            for (int i = 1; i <= POTLR; ++i)
                if (singlr[i - 1] && TP(i, t0) == 1) TP(i, t0) = 0;

            // Add (ADDALL) or track (ADDONE) the largest significant outlier.
            bool otlrno = true;
            int itype = 1;
            while (otlrno && itype <= POTLR) {
                int mt = mxtype[itype - 1];
                if (mt != NOTSET) {
                    if (std::fabs(tvalt0[mt - 1]) > critt[mt - 1] &&
                        TP(mt, t0) == 1) {
                        std::string ttl = wrtotl(mt, t0, itmp, D.begspn.data(), sp);
                        double otlb = std::copysign(tvalt0[mt - 1] * tvalt0[mt - 1],
                                                    tvalt0[mt - 1]);
                        if (!ladd1) {
                            adrgef(ctx, otlb, ttl, AOTLTL, mxcode[mt - 1], false,
                                   false);
                            if (ctx.error.lfatal) return;
                            if (M.iregfx == 3) M.iregfx = 2;
                            TP(mt, t0) = 0;
                        } else if (std::fabs(tvalt0[mt - 1]) > std::fabs(mxotlt)) {
                            mxotl = ttl;
                            mxotlb = otlb;
                            mxotlt = tvalt0[mt - 1];
                            mxott0 = t0;
                            mxottp = mt;
                        }
                        otlrno = false;
                    }
                }
                ++itype;
            }
        }

        // ADDONE: add the single most significant outlier found this pass.
        if (ladd1 && std::fabs(mxotlt) > ZERO) {
            adrgef(ctx, mxotlb, mxotl, AOTLTL, mxcode[mxottp - 1], false, false);
            if (ctx.error.lfatal) return;
            if (M.iregfx == 3) M.iregfx = 2;
            TP(mxottp, mxott0) = 0;
        }

        // Recount the auto-outlier group.
        otlgrp = find_otlgrp();
        int natotl = 0;
        if (otlgrp > 0) {
            eltlen(ctx, otlgrp, M.grp.data(), M.ngrp, natotl);
            if (ctx.error.lfatal) return;
        }
        if (natotl <= 0) return;   // no outliers at all -> done
        int newotl = natotl - oldotl;
        if (newotl <= 0) break;    // nothing new this pass -> backward deletion

        // Make room, rebuild the outlier columns, and re-estimate.
        int begcol = M.grp(otlgrp - 1);
        int endcol = begcol + newotl - 1;
        int coloc = oldnc;
        coladd(begcol, endcol, nspobs, PXY, D.xy.data(), coloc);
        endcol = M.grp(otlgrp) - 1;
        addotl(ctx, D.begspn.data(), nspobs, 0, begcol, endcol);
        if (ctx.error.lfatal) return;
        if (lxreg) {
            int nn = 0;
            regx11(ctx, a, &nn, &nefobs);
            prterx_if_singular(ctx);        // idotlr.f:878
            rgtdhl(ctx);                    // idotlr.f:879
            if (ctx.error.lfatal) return;
            na = nn;
        } else {
            rgarma(ctx, lestim, mxiter, mxnlit, false, a, na, nefobs, lautmp);
        }
        if (ctx.error.lfatal) return;
        if (!D.convrg) {
            ctx.error.lfatal = true;
            return;
        }
        eltlen(ctx, otlgrp, M.grp.data(), M.ngrp, oldotl);
        if (ctx.error.lfatal) return;
    }

    // ---- Backward deletion ---------------------------------------------------
    double mint[POTLR];
    int mini[POTLR], minptr[POTLR];
    while (true) {
        int begcol = M.grp(otlgrp - 1);
        int endcol = M.grp(otlgrp) - 1;
        deltst(ctx, nefobs, begcol, endcol, mint, mini, minptr, lauto, false);
        if (ctx.error.lfatal) return;

        bool deleted = false;
        for (int iptr = 1; iptr <= POTLR && !deleted; ++iptr) {
            if (minptr[iptr - 1] == NOTSET) continue;
            int iptr2 = minptr[iptr - 1];
            if (std::fabs(mint[iptr2 - 1]) < critvl[iptr2 - 1]) {
                int col = mini[iptr2 - 1];
                dlrgef(ctx, col, nspobs, 1);
                if (ctx.error.lfatal) return;
                int na = 0;
                if (lxreg) {
                    regx11(ctx, a, &na, &nefobs);
                    prterx_if_singular(ctx);        // idotlr.f:995
                    rgtdhl(ctx);                    // idotlr.f:996
                    if (ctx.error.lfatal) return;
                } else {
                    rgarma(ctx, lestim, mxiter, mxnlit, false, a, na, nefobs,
                           lautmp);
                }
                if (ctx.error.lfatal) return;
                if (!D.convrg) {
                    ctx.error.lfatal = true;
                    return;
                }
                otlgrp = find_otlgrp();
                if (otlgrp > 0) {
                    deleted = true;   // more outliers remain -> next pass
                } else {
                    return;           // deleted the last outlier -> done
                }
            }
        }
        if (!deleted) return;   // no outlier below the critical value -> done
    }
}

}  // namespace x13
