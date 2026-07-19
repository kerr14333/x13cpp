// estrmu.cpp -- estrmu.f: long-term (or span-computed) monthly means of the
// Easter regressor, used to center the Easter column against the trend.
#include "regarima/regvar.hpp"
#include "specparse/specparse.hpp"
#include "srslen.hpp"

#include <vector>

namespace x13 {

void estrmu(const int* begdat, int nrxy, int sp, int ndays, bool hlong,
            double* hmean, bool hstock) {
    constexpr int MO = 2;
    constexpr double ZERO = 0.0;

    // Means of Easter regressors from a 500 year span (1600-2099); emeans(26,2:4).
    static const double emeans2[26] = {
        0.00368, 0.002083333, 0.001130435, 0.0002727273, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    static const double emeans3[26] = {
        0.6576,    0.6450833, 0.6311304, 0.6162727, 0.5999048,
        0.583,     0.5661053, 0.549,     0.5318824, 0.514625,
        0.4973333, 0.4807143, 0.4643077, 0.4476667, 0.4305455,
        0.4136,    0.3975556, 0.382,     0.3654286, 0.3483333,
        0.3304,    0.3125,    0.2966667, 0.281,     0.266,     0.232};
    static const double emeans4[26] = {
        0.33872,   0.3528333, 0.3677391, 0.3834545, 0.4000952,
        0.417,     0.4338947, 0.451,     0.4681176, 0.485375,
        0.5026667, 0.5192857, 0.5356923, 0.5523333, 0.5694545,
        0.5864,    0.6024444, 0.618,     0.6345714, 0.6516667,
        0.6696,    0.6875,    0.7033333, 0.719,     0.734,     0.768};
    auto emeans = [&](int i, int j) -> double {
        return j == 2 ? emeans2[i - 1] : (j == 3 ? emeans3[i - 1] : emeans4[i - 1]);
    };

    if (hlong) {
        // Long term means from the previous version of X-13ARIMA-SEATS.
        int mnindx = 25 - ndays + 1;
        if (sp == 4) {
            for (int i = 1; i <= sp; ++i) {
                if (i == 1) hmean[i - 1] = emeans(mnindx, 2) + emeans(mnindx, 3);
                else if (i == 2) hmean[i - 1] = emeans(mnindx, 4);
                else hmean[i - 1] = ZERO;
            }
        } else {
            for (int i = 1; i <= sp; ++i) {
                if (i >= 2 && i <= 4) hmean[i - 1] = emeans(mnindx, i);
                else hmean[i - 1] = ZERO;
            }
        }
    } else {
        // Ensure that full years are used in computing Hmean.
        int tdat[2];
        cpyint(begdat, 2, 1, tdat);
        int n2 = nrxy;
        if (tdat[MO - 1] > 1) {
            n2 = n2 + tdat[MO - 1] - 1;
            tdat[MO - 1] = 1;
        }
        int n3 = n2 % sp;
        if (n3 > 0) n2 = n2 + sp - n3;

        std::vector<double> txy(static_cast<std::size_t>(prm::PLEN), 0.0);
        adestr(tdat, n2, 1, sp, 1, ndays, 0, txy.data(), false, hmean, hstock);

        for (int i = 1; i <= sp; ++i) {
            hmean[i - 1] = ZERO;
            if ((sp == 12 && (i >= 2 && i <= 4)) ||
                (sp == 4 && (i == 1 || i == 2))) {
                int i1 = i;
                int n = 1;
                while (n <= 29 && i1 <= n2) {
                    hmean[i - 1] = hmean[i - 1] + txy[static_cast<std::size_t>(i1 - 1)];
                    i1 = i1 + sp;
                    n = n + 1;
                }
                hmean[i - 1] = hmean[i - 1] / static_cast<double>(n - 1);
            }
        }
    }
}

}  // namespace x13
