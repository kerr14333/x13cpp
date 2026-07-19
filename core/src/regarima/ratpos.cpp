// ratpos.cpp -- ratpos.f (power-series expansion of a rational polynomial,
// used to integrate the Constant column against the model differencing),
// copycl.f (matrix column copy), gtrgpt.f (change-of-regime pointer vector).
#include "regarima/regvar.hpp"
#include "specparse/specparse.hpp"
#include "srslen.hpp"

#include <cmath>

namespace x13 {

// ratpos.f
void ratpos(int nelta, const double* arimap, const int* arimal, const int* opr,
            int begopr, int endopr, int neltc, double* c) {
    constexpr double ZERO = 0.0;
    int ntmpa = nelta;
    for (int iopr = begopr; iopr <= endopr; ++iopr) {
        int beglag = opr[iopr - 1];
        int endlag = opr[iopr] - 1;
        int begelt;
        if (endlag > beglag) begelt = arimal[beglag - 1] + 1;
        else begelt = 1;
        for (int i = ntmpa + 1; i <= begelt - 1; ++i) c[i - 1] = ZERO;
        // Calculate the c(i)'s.
        for (int i = begelt; i <= neltc; ++i) {
            double sum;
            if (i <= ntmpa) sum = c[i - 1];
            else sum = ZERO;
            for (int ilag = beglag; ilag <= endlag; ++ilag) {
                int itmp = i - arimal[ilag - 1];
                if (itmp > 0) {
                    if (std::fabs(arimap[ilag - 1]) > 1.0e-150 &&
                        std::fabs(c[itmp - 1]) > 1.0e-150)
                        sum = sum + arimap[ilag - 1] * c[itmp - 1];
                }
            }
            c[i - 1] = sum;
        }
        ntmpa = neltc;
    }
}

// copycl.f
void copycl(const double* from, int nr, int nfrmcl, int ifrmcl, int ntocl,
            int itocl, double* to) {
    int eltfrm = ifrmcl - nfrmcl;
    int eltto = itocl - ntocl;
    for (int i = 1; i <= nr; ++i) {
        eltfrm = eltfrm + nfrmcl;
        eltto = eltto + ntocl;
        to[eltto - 1] = from[eltfrm - 1];
    }
}

// gtrgpt.f
void gtrgpt(X13Context& ctx, const int* begdat, const int* rgdate, int rgzero,
            bool* rgdtvc, int nobs) {
    setlg(false, prm::PLEN, rgdtvc);
    int rgmidx;
    dfdate(rgdate, begdat, ctx.model.sp, rgmidx);
    rgmidx = rgmidx + 1;
    if (rgzero == 1) {
        if (rgmidx > 0) {
            for (int i = 1; i <= rgmidx - 1; ++i)
                if (!rgdtvc[i - 1]) rgdtvc[i - 1] = true;
        }
    } else {
        if (rgmidx <= 0) rgmidx = 1;
        for (int i = rgmidx; i <= nobs; ++i)
            if (!rgdtvc[i - 1]) rgdtvc[i - 1] = true;
    }
}

}  // namespace x13
