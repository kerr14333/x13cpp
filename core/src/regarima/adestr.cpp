// adestr.cpp -- adestr.f + sceast.f: the Bell Easter holiday regressor (and
// the Statistics Canada variant) into one column of Xy.
#include "regarima/regvar.hpp"
#include "specparse/specparse.hpp"
#include "gen/model.hpp"

namespace x13 {

// sceast.f
double sceast(int ndays, int pdays, bool first, bool ineast) {
    constexpr double ONE = 1.0, ZERO = 0.0;
    if (first) {
        if (pdays == ndays || ineast) return ONE;
        return static_cast<double>(pdays) / static_cast<double>(ndays);
    }
    if (pdays == ndays) return ZERO;
    return static_cast<double>(pdays - ndays) / static_cast<double>(ndays);
}

// adestr.f
void adestr(const int* begdat, int nrxy, int ncxy, int isp, int icol, int ndays,
            int easidx, double* xy, bool xmeans, const double* emean, bool estock) {
    using namespace prm;
    constexpr double ZERO = 0.0, MONE = -1.0;

    // The date of Easter = March 22 + kdate(year), year = 1901..2100.
    static const int kdate[200] = {
        16,  8, 21, 12, 32, 24,  9, 28, 20,  5, 25, 16,  1, 21, 13, 32, 17,  9, 29, 13,
         5, 25, 10, 29, 21, 13, 26, 17,  9, 29, 14,  5, 25, 10, 30, 21,  6, 26, 18,  2,
        22, 14, 34, 18, 10, 30, 15,  6, 26, 18,  3, 22, 14, 27, 19, 10, 30, 15,  7, 26,
        11, 31, 23,  7, 27, 19,  4, 23, 15,  7, 20, 11, 31, 23,  8, 27, 19,  4, 24, 15,
        28, 20, 12, 31, 16,  8, 28, 12,  4, 24,  9, 28, 20, 12, 25, 16,  8, 21, 13, 32,
        24,  9, 29, 20,  5, 25, 17,  1, 21, 13, 33, 17,  9, 29, 14,  5, 25, 10, 30, 21,
        13, 26, 18,  9, 29, 14,  6, 25, 10, 30, 22,  6, 26, 18,  3, 22, 14, 34, 19, 10,
        30, 15,  7, 26, 18,  3, 23, 14, 27, 19, 11, 30, 15,  7, 27, 11, 31, 23,  8, 27,
        19,  4, 24, 15,  7, 20, 12, 31, 23,  8, 28, 19,  4, 24, 16, 28, 20, 12, 32, 16,
         8, 28, 13,  4, 24,  9, 29, 20, 12, 25, 17,  8, 21, 13, 33, 24,  9, 29, 21,  6};
    // Cumulative sums of lengths of months / quarters: cmlnmo(13,2), cmlnqt(5,2).
    static const int cmlnmo[13][2] = {{0, 0},     {31, 31},   {59, 60},
                                      {90, 91},   {120, 121}, {151, 152},
                                      {181, 182}, {212, 214}, {243, 244},
                                      {273, 274}, {304, 305}, {334, 335},
                                      {365, 366}};
    static const int cmlnqt[5][2] = {{0, 0}, {90, 91}, {181, 182}, {273, 274},
                                     {365, 366}};

    int predat[2];
    addate(begdat, isp, -1, predat);

    auto XY = [&](int c, int r) -> double& {
        return xy[static_cast<std::size_t>((r - 1) * ncxy + (c - 1))];
    };

    for (int i = 1; i <= nrxy; ++i) {
        int idate[2];
        addate(predat, isp, i, idate);
        int year = idate[YR - 1];
        int period = idate[MO - 1];
        int lpyr;
        if ((year % 100 != 0 && year % 4 == 0) || year % 400 == 0) lpyr = 2;
        else lpyr = 1;

        double tmp;
        if (isp != 12) {
            // Quarterly Easter effect.
            int julbeg = cmlnqt[period - 1][lpyr - 1] + 1;
            int julend = cmlnqt[period][lpyr - 1];
            int juleas = cmlnqt[1][lpyr - 1] - 9 + kdate[year - 1901];
            int ibeg, iend;
            if (ndays > 0) {
                ibeg = std::max(julbeg, juleas - ndays + easidx);
                iend = std::min(julend, juleas - 1 + easidx);
            } else {
                ibeg = std::max(julbeg, juleas - easidx);
                iend = std::min(julend, juleas - easidx);
            }
            if (ibeg <= iend) {
                int itmp = iend - ibeg + 1;
                if (easidx == 0) {
                    tmp = static_cast<double>(itmp);
                    if (ndays > 0) tmp = tmp / static_cast<double>(ndays);
                } else {
                    tmp = sceast(ndays, itmp, period == 1, julend >= juleas);
                }
            } else {
                if (easidx == 1 && period == 2) tmp = MONE;
                else tmp = ZERO;
            }
        } else {
            // Monthly Easter effect.
            int julbeg = cmlnmo[period - 1][lpyr - 1] + 1;
            int julend = cmlnmo[period][lpyr - 1];
            int juleas = cmlnmo[2][lpyr - 1] + 22 + kdate[year - 1901];
            int ibeg, iend;
            if (ndays > 0) {
                ibeg = std::max(julbeg, juleas - ndays + easidx);
                iend = std::min(julend, juleas - 1 + easidx);
            } else {
                ibeg = std::max(julbeg, juleas - easidx);
                iend = std::min(julend, juleas - easidx);
            }
            if (ibeg <= iend) {
                int itmp = iend - ibeg + 1;
                if (easidx == 0) {
                    tmp = static_cast<double>(itmp);
                    if (ndays > 0) tmp = tmp / static_cast<double>(ndays);
                } else {
                    tmp = sceast(ndays, itmp, period == 3, julend >= juleas);
                }
            } else {
                if (easidx == 1 && period == 4) tmp = MONE;
                else tmp = ZERO;
            }
        }

        // Subtract off long term means (orthogonal to the trend).
        if (xmeans && easidx == 0) tmp = tmp - emean[period - 1];

        // Stock end-of-month easter.
        if (estock) {
            if (isp == 4) {
                if (period == 2) tmp = ZERO;
            } else {
                if (period == 3) tmp = tmp + XY(icol, i - 1);
                else if (period == 4) tmp = ZERO;
            }
        }

        XY(icol, i) = tmp;
    }
}

}  // namespace x13
