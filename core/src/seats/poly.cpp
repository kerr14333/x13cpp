// poly.cpp -- SEATS polynomial arithmetic: CONV / CONJ / MULTFN / DIVFCN.
// Faithful port of oracle/fortran/ansub2.f (lines 341-579). The accumulation
// order of CONV/CONJ is preserved exactly; SEATS uses these (not the regARIMA
// uconv) so its component decomposition stays bit-for-bit with the oracle.
//
// The Fortran uses 1-based arrays; here the incoming pointers are 0-based C
// arrays, so every Fortran subscript X(i) becomes X[i-1]. Local scratch mirrors
// the Fortran local arrays (d(500), c1/c2(165), wr(100)).
#include "seats/seatspoly.hpp"

#include <cmath>
#include <algorithm>

namespace x13 {

// CONV(a,mplus1,b,nplus1,c,lplus1) -- ansub2.f:341
void conv(const double* a, int mplus1, const double* b, int nplus1,
          double* c, int& lplus1) {
    double d[500];
    int jplus1 = nplus1;
    lplus1 = mplus1 + jplus1 - 1;
    for (int i = 1; i <= jplus1; ++i) d[i - 1] = b[i - 1];
    for (int i = 1; i <= lplus1; ++i) c[i - 1] = 0.0;
    for (int i = 1; i <= mplus1; ++i) {
        for (int j = 1; j <= jplus1; ++j) {
            int num = i + j - 1;
            c[num - 1] = c[num - 1] + a[i - 1] * d[j - 1];
        }
    }
}

// CONJ(a,mplus1,b,nplus1,c,lplus1) -- ansub2.f:400
void conj(const double* a, int mplus1, const double* b, int nplus1,
          double* c, int& lplus1) {
    lplus1 = std::max(mplus1, nplus1);
    for (int i = 1; i <= lplus1; ++i) c[i - 1] = 0.0;
    for (int i = 1; i <= mplus1; ++i) {
        for (int j = 1; j <= nplus1; ++j) {
            int k = i - j;
            int num = std::abs(k) + 1;
            c[num - 1] = c[num - 1] + a[i - 1] * b[j - 1];
        }
    }
}

// MULTFN(a,mplus1,b,nplus1,c,lplus1) -- ansub2.f:455
void multfn(const double* a, int mplus1, const double* b, int nplus1,
            double* c, int& lplus1) {
    double c1[165], c2[165];
    int l1, l2;
    lplus1 = mplus1 + nplus1 - 1;
    for (int i = 1; i <= lplus1; ++i) c2[i - 1] = 0.0;
    conv(a, mplus1, b, nplus1, c1, l1);
    conj(a, mplus1, b, nplus1, c2, l2);
    for (int i = 1; i <= l1; ++i) {
        c[i - 1] = (c1[i - 1] + c2[i - 1]) / 2;
    }
}

// DIVFCN(a,mplus1,b,nplus1,q,nq,r,nr) -- ansub2.f:516
void divfcn(const double* a, int mplus1, const double* b, int nplus1,
            double* q, int& nq, double* r, int& nr) {
    double wr[100];
    nq = mplus1 - nplus1 + 1;
    for (int i = 1; i <= mplus1; ++i) {
        wr[i - 1] = a[i - 1];
        if (i <= nq) {
            q[i - 1] = 0.0;
        }
    }
    if (nq >= 1) {
        if (nplus1 == 1) {
            for (int i = 1; i <= mplus1; ++i) {
                q[i - 1] = a[i - 1] / b[0];
            }
            r[0] = 0.0;
            nr = 0;
            return;
        } else {
            for (int kprime = 1; kprime <= nq; ++kprime) {
                int k = nq - kprime;
                int num = mplus1 - kprime + 1;
                double factor = wr[num - 1] / b[nplus1 - 1];
                q[k + 1 - 1] = 2.0 * factor;
                if (k == 0) {
                    q[k + 1 - 1] = factor;
                }
                for (int i = 1; i <= nplus1; ++i) {
                    int j = i + k;
                    int jp = std::abs(i - k - 1) + 1;
                    wr[j - 1] = wr[j - 1] - factor * b[i - 1];
                    if (k != 0) {
                        wr[jp - 1] = wr[jp - 1] - factor * b[i - 1];
                    }
                }
            }
        }
    } else {
        nq = 0;
    }
    nr = nplus1 - 1;
    for (int i = 1; i <= nr; ++i) {
        r[i - 1] = wr[i - 1];
    }
}

}  // namespace x13
