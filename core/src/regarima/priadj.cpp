// priadj.cpp -- length-of-period / leap-year prior factors (td7var.f, Mltadd).
#include "regarima/priadj.hpp"

namespace x13 {

namespace {
// td7var.f DATA lnomo/lnoqtr: days per month/quarter, [period][lpyr-1].
constexpr int LNOMO[12][2] = {{31, 31}, {28, 29}, {31, 31}, {30, 30},
                              {31, 31}, {30, 30}, {31, 31}, {31, 31},
                              {30, 30}, {31, 31}, {30, 30}, {31, 31}};
constexpr int LNOQTR[4][2] = {{90, 91}, {91, 91}, {92, 92}, {92, 92}};

constexpr double AVEMO = 30.4375, AVEQTR = 91.3125;
constexpr double P28FEB = 28.0 / 28.25, P29FEB = 29.0 / 28.25;
constexpr double P90QTR = 90.0 / 90.25, P91QTR = 91.0 / 90.25;
}  // namespace

int lpyr_index(int year) {
    bool leap = ((year % 100 != 0 && year % 4 == 0) || year % 400 == 0);
    return leap ? 2 : 1;
}

double lpfac(int year, int period, int sp, bool lom) {
    int lpyr = lpyr_index(year);   // 1 or 2

    if (sp != 12) {
        // Quarterly (td7var.f Isp!=12 branch, Mltadd=.true.).
        if (lom) {
            int ndoqtr = LNOQTR[period - 1][lpyr - 1];
            return static_cast<double>(ndoqtr) / AVEQTR;
        }
        if (period != 1) return 1.0;
        return (lpyr == 2) ? P91QTR : P90QTR;
    }

    // Monthly (td7var.f Isp==12 branch, Mltadd=.true.).
    if (lom) {
        int ndomo = LNOMO[period - 1][lpyr - 1];
        return static_cast<double>(ndomo) / AVEMO;
    }
    if (period != 2) return 1.0;
    return (lpyr == 2) ? P29FEB : P28FEB;
}

}  // namespace x13
