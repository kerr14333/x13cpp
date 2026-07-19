// armafilt.cpp -- ARMA filter routines over the model-operator structure
// (see armafilt.hpp). Faithful ports of the vendored oracle Fortran.
#include "regarima/armafilt.hpp"

#include <vector>

#include "specparse/specparse.hpp"  // maxlag

namespace x13 {

// arflt.f -- ntmpa shrinks by each operator's max lag; C(i) written while
// C(off) and C(off-Arimal(ilag)) are read ahead (ascending i is load-bearing).
void arflt(int nelta, const double* arimap, const int* arimal, const int* opr,
           int begopr, int endopr, double* c, int& neltc) {
    int ntmpa = nelta;
    for (int iopr = begopr; iopr <= endopr; ++iopr) {
        int mxlag;
        maxlag(arimal, opr, iopr, iopr, mxlag);
        ntmpa = ntmpa - mxlag;
        int beglag = opr[iopr - 1];
        int endlag = opr[iopr] - 1;
        for (int i = 1; i <= ntmpa; ++i) {
            int off = i + mxlag;
            double tmp = c[off - 1];
            for (int ilag = beglag; ilag <= endlag; ++ilag) {
                tmp = tmp - arimap[ilag - 1] * c[off - arimal[ilag - 1] - 1];
            }
            c[i - 1] = tmp;
        }
        neltc = ntmpa;
    }
}

// mltpos.f -- work buffer (indices 1..neltc), copy work->c after each operator;
// secpas gates the zero-padding on the first operator pass only.
void mltpos(int nelta, const double* arimap, const int* arimal, const int* opr,
            int begopr, int endopr, int neltc, double* c) {
    constexpr double ZERO = 0.0;
    std::vector<double> work(neltc > 0 ? neltc : 1, ZERO);
    bool secpas = false;
    for (int iopr = begopr; iopr <= endopr; ++iopr) {
        int beglag = opr[iopr - 1];
        int endlag = opr[iopr] - 1;
        for (int i = 1; i <= neltc; ++i) {
            double tmp;
            if (secpas || i <= nelta) tmp = c[i - 1];
            else tmp = ZERO;
            for (int ilag = beglag; ilag <= endlag; ++ilag) {
                int itmp = i - arimal[ilag - 1];
                if (itmp > 0 && (secpas || itmp <= nelta))
                    tmp = tmp - arimap[ilag - 1] * c[itmp - 1];
            }
            work[i - 1] = tmp;
        }
        secpas = true;
        for (int i = 1; i <= neltc; ++i) c[i - 1] = work[i - 1];
    }
}

}  // namespace x13
