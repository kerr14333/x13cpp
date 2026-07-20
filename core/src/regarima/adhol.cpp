// adhol.cpp -- adlabr.f + adthnk.f: Bell's Labor-Day and Thanksgiving-Christmas
// holiday regressors, each built into one column of Xy. Monthly-only (isp=12).
#include "regarima/regvar.hpp"
#include "specparse/specparse.hpp"
#include "gen/model.hpp"

namespace x13 {

// adlabr.f -- Labor Day holiday variable (Bell 1983).
void adlabr(const int* begdat, int nrxy, int ncxy, int icol, int ndays,
            double* xy, bool xmeans) {
    using namespace prm;
    constexpr double ZERO = 0.0;

    // Date of Labor Day = August 31 + kdate(year), year = 1901..2100.
    static const int kdate[200] = {
        2, 1, 7, 5, 4, 3, 2, 7, 6, 5, 4, 2, 1, 7, 6, 4, 3, 2, 1, 6, 5, 4, 3, 1, 7,
        6, 5, 3, 2, 1, 7, 5, 4, 3, 2, 7, 6, 5, 4, 2, 1, 7, 6, 4, 3, 2, 1, 6, 5, 4,
        3, 1, 7, 6, 5, 3, 2, 1, 7, 5, 4, 3, 2, 7, 6, 5, 4, 2, 1, 7, 6, 4, 3, 2, 1,
        6, 5, 4, 3, 1, 7, 6, 5, 3, 2, 1, 7, 5, 4, 3, 2, 7, 6, 5, 4, 2, 1, 7, 6, 4,
        3, 2, 1, 6, 5, 4, 3, 1, 7, 6, 5, 3, 2, 1, 7, 5, 4, 3, 2, 7, 6, 5, 4, 2, 1,
        7, 6, 4, 3, 2, 1, 6, 5, 4, 3, 1, 7, 6, 5, 3, 2, 1, 7, 5, 4, 3, 2, 7, 6, 5,
        4, 2, 1, 7, 6, 4, 3, 2, 1, 6, 5, 4, 3, 1, 7, 6, 5, 3, 2, 1, 7, 5, 4, 3, 2,
        7, 6, 5, 4, 2, 1, 7, 6, 4, 3, 2, 1, 6, 5, 4, 3, 1, 7, 6, 5, 3, 2, 1, 7, 5};
    static const int cmlnmo[13][2] = {{0, 0},     {31, 31},   {59, 60},
                                      {90, 91},   {120, 121}, {151, 152},
                                      {181, 182}, {212, 214}, {243, 244},
                                      {273, 274}, {304, 305}, {334, 335},
                                      {365, 366}};
    // Long-term means of holiday effects: [tau][period-8], period 8 (Aug) / 9 (Sep).
    static const double means[25][2] = {
        {.8800, .1200}, {.8750, .1250}, {.8696, .1304}, {.8636, .1364},
        {.8571, .1429}, {.8500, .1500}, {.8421, .1579}, {.8333, .1667},
        {.8235, .1765}, {.8125, .1875}, {.8000, .2000}, {.7857, .2143},
        {.7692, .2308}, {.7500, .2500}, {.7273, .2727}, {.7000, .3000},
        {.6667, .3333}, {.6250, .3750}, {.5714, .4286}, {.5000, .5000},
        {.4286, .5714}, {.3571, .6429}, {.2857, .7143}, {.2143, .7857},
        {.1429, .8571}};

    int mnindx = 25 - ndays + 1;
    int predat[2];
    addate(begdat, 12, -1, predat);

    auto XY = [&](int c, int r) -> double& {
        return xy[static_cast<std::size_t>((r - 1) * ncxy + (c - 1))];
    };

    for (int i = 1; i <= nrxy; ++i) {
        int idate[2];
        addate(predat, 12, i, idate);
        int year = idate[YR - 1];
        int period = idate[MO - 1];
        int lpyr;
        if ((year % 100 != 0 && year % 4 == 0) || year % 400 == 0) lpyr = 2;
        else lpyr = 1;

        int julbeg = cmlnmo[period - 1][lpyr - 1] + 1;
        int julend = cmlnmo[period][lpyr - 1];
        int jullab = cmlnmo[8][lpyr - 1] + kdate[year - 1901];
        int ibeg = std::max(julbeg, jullab - ndays);
        int iend = std::min(julend, jullab - 1);

        double tmp;
        if (ibeg <= iend) tmp = static_cast<double>(iend - ibeg + 1) / static_cast<double>(ndays);
        else tmp = ZERO;

        if (xmeans && (period == 8 || period == 9))
            tmp = tmp - means[mnindx - 1][period - 8];

        XY(icol, i) = tmp;
    }
}

// adthnk.f -- Thanksgiving-Christmas holiday variable (Bell 1983).
void adthnk(const int* begdat, int nrxy, int ncxy, int icol, int ndays,
            double* xy, bool xmeans) {
    using namespace prm;
    constexpr double ZERO = 0.0;

    // Date of Thanksgiving = November 21 + kdate(year), year = 1901..2100.
    static const int kdate[200] = {
        7, 6, 5, 3, 2, 1, 7, 5, 4, 3, 2, 7, 6, 5, 4, 2, 1, 7, 6, 4, 3, 2, 1, 6, 5,
        4, 3, 1, 7, 6, 5, 3, 2, 1, 7, 5, 4, 3, 2, 7, 6, 5, 4, 2, 1, 7, 6, 4, 3, 2,
        1, 6, 5, 4, 3, 1, 7, 6, 5, 3, 2, 1, 7, 5, 4, 3, 2, 7, 6, 5, 4, 2, 1, 7, 6,
        4, 3, 2, 1, 6, 5, 4, 3, 1, 7, 6, 5, 3, 2, 1, 7, 5, 4, 3, 2, 7, 6, 5, 4, 2,
        1, 7, 6, 4, 3, 2, 1, 6, 5, 4, 3, 1, 7, 6, 5, 3, 2, 1, 7, 5, 4, 3, 2, 7, 6,
        5, 4, 2, 1, 7, 6, 4, 3, 2, 1, 6, 5, 4, 3, 1, 7, 6, 5, 3, 2, 1, 7, 5, 4, 3,
        2, 7, 6, 5, 4, 2, 1, 7, 6, 4, 3, 2, 1, 6, 5, 4, 3, 1, 7, 6, 5, 3, 2, 1, 7,
        5, 4, 3, 2, 7, 6, 5, 4, 2, 1, 7, 6, 4, 3, 2, 1, 6, 5, 4, 3, 1, 7, 6, 5, 3};
    static const int cmlnmo[13][2] = {{0, 0},     {31, 31},   {59, 60},
                                      {90, 91},   {120, 121}, {151, 152},
                                      {181, 182}, {212, 214}, {243, 244},
                                      {273, 274}, {304, 305}, {334, 335},
                                      {365, 366}};
    // Long-term means: [tau][period-11], period 11 (Nov) / 12 (Dec).
    static const double means[25][2] = {
        {.4884, .5116}, {.4773, .5227}, {.4656, .5344}, {.4534, .5466},
        {.4406, .5594}, {.4273, .5727}, {.4132, .5868}, {.3985, .6015},
        {.3830, .6170}, {.3667, .6333}, {.3494, .6506}, {.3313, .6687},
        {.3120, .6880}, {.2917, .7083}, {.2700, .7300}, {.2471, .7529},
        {.2226, .7774}, {.1684, .8316}, {.1384, .8616}, {.1062, .8938},
        {.0776, .9224}, {.0530, .9470}, {.0326, .9674}, {.0167, .9833},
        {.0057, .9943}};

    int mnindx = 18 - ndays;
    if (ndays < 0) mnindx = mnindx - 1;
    int predat[2];
    addate(begdat, 12, -1, predat);

    auto XY = [&](int c, int r) -> double& {
        return xy[static_cast<std::size_t>((r - 1) * ncxy + (c - 1))];
    };

    for (int i = 1; i <= nrxy; ++i) {
        int idate[2];
        addate(predat, 12, i, idate);
        int year = idate[YR - 1];
        int period = idate[MO - 1];
        int lpyr;
        if ((year % 100 != 0 && year % 4 == 0) || year % 400 == 0) lpyr = 2;
        else lpyr = 1;

        int julbeg = cmlnmo[period - 1][lpyr - 1] + 1;
        int julend = cmlnmo[period][lpyr - 1];
        int julthk = cmlnmo[10][lpyr - 1] + 21 + kdate[year - 1901];
        int ibeg = std::max(julbeg, julthk - ndays);
        int julxms = cmlnmo[11][lpyr - 1] + 25;
        int iend = std::min(julend, julxms - 1);

        int win = julxms - julthk + ndays;
        double tmp;
        if (ibeg <= iend) tmp = static_cast<double>(iend - ibeg + 1) / static_cast<double>(win);
        else tmp = ZERO;

        if (xmeans && (period == 11 || period == 12))
            tmp = tmp - means[mnindx - 1][period - 11];

        XY(icol, i) = tmp;
    }
}

}  // namespace x13
