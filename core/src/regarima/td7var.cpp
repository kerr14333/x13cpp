// td7var.cpp -- td7var.f: the seventh trading-day variable (length-of-month/
// quarter, leap year, or stock length-of-month) into one column of Xy. The
// multiplicative branch is also exposed pre-model as lpfac() (priadj.cpp).
#include "regarima/regvar.hpp"
#include "specparse/specparse.hpp"
#include "gen/model.hpp"

namespace x13 {

void td7var(const int* begdat, int isp, int nrxy, int ncxy, int begcol, bool lom,
            bool ltdstk, bool mltadd, double* xy, const bool* begrgm) {
    using namespace prm;
    constexpr double ONE = 1.0, ZERO = 0.0, MP25 = -0.25, P75 = 0.75;
    constexpr double AVEMO = 30.4375, AVEQTR = 91.3125;
    constexpr double P28FEB = 28.0 / 28.25;
    constexpr double P29FEB = 29.0 / 28.25;
    constexpr double P90QTR = 90.0 / 90.25;
    constexpr double P91QTR = 91.0 / 90.25;

    static const int lnomo[12][2] = {{31, 31}, {28, 29}, {31, 31}, {30, 30},
                                     {31, 31}, {30, 30}, {31, 31}, {31, 31},
                                     {30, 30}, {31, 31}, {30, 30}, {31, 31}};
    static const int lnoqtr[4][2] = {{90, 91}, {91, 91}, {92, 92}, {92, 92}};
    // sly(48): -.375, 12*.375, 12*.125, 12*-.125, 11*-.375
    static const double sly[48] = {
        -.375, .375, .375, .375, .375, .375, .375, .375, .375, .375, .375, .375,
        .375,  .125, .125, .125, .125, .125, .125, .125, .125, .125, .125, .125,
        .125,  -.125, -.125, -.125, -.125, -.125, -.125, -.125, -.125, -.125,
        -.125, -.125, -.125, -.375, -.375, -.375, -.375, -.375, -.375, -.375,
        -.375, -.375, -.375, -.375};

    int predat[2];
    addate(begdat, isp, -1, predat);

    auto XY = [&](int icol, int irow) -> double& {
        return xy[static_cast<std::size_t>((irow - 1) * ncxy + (icol - 1))];
    };

    for (int irow = 1; irow <= nrxy; ++irow) {
        int idate[2];
        addate(predat, isp, irow, idate);
        int year = idate[YR - 1];
        int period = idate[MO - 1];

        int idxsly = 0;
        if (ltdstk) idxsly = (year % 4) * isp + period;
        int lpyr;
        if ((year % 100 != 0 && year % 4 == 0) || year % 400 == 0) lpyr = 2;
        else lpyr = 1;

        double td7;
        if (isp != 12) {
            if (lom) {
                int ndoqtr = lnoqtr[period - 1][lpyr - 1];
                if (mltadd) td7 = static_cast<double>(ndoqtr) / AVEQTR;
                else td7 = static_cast<double>(ndoqtr) - AVEQTR;
            } else if (mltadd) {
                if (period != 1) td7 = ONE;
                else if (lpyr == 2) td7 = P91QTR;
                else td7 = P90QTR;
            } else if (period != 1) {
                td7 = ZERO;
            } else if (lpyr == 2) {
                td7 = P75;
            } else {
                td7 = MP25;
            }
        } else if (lom) {
            int ndomo = lnomo[period - 1][lpyr - 1];
            if (mltadd) td7 = static_cast<double>(ndomo) / AVEMO;
            else td7 = static_cast<double>(ndomo) - AVEMO;
        } else if (mltadd) {
            if (period != 2) td7 = ONE;
            else if (lpyr == 2) td7 = P29FEB;
            else td7 = P28FEB;
        } else if (period != 2) {
            td7 = ZERO;
        } else if (lpyr == 2) {
            td7 = P75;
        } else {
            td7 = MP25;
        }

        // Stock length-of-month variable.
        if (ltdstk && !mltadd) td7 = sly[idxsly - 1];

        if (mltadd) XY(begcol, irow) = ONE;
        else XY(begcol, irow) = ZERO;
        if (begrgm[irow - 1]) XY(begcol, irow) = td7;
    }
}

}  // namespace x13
