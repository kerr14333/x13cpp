// td6var.cpp -- td6var.f: the first six trading-day variables (day-of-week
// contrasts vs Sunday), the one-coefficient weekday variable, and their stock
// counterparts, inserted into the regression matrix Xy.
#include "regarima/regvar.hpp"
#include "specparse/specparse.hpp"
#include "gen/model.hpp"

namespace x13 {

void td6var(X13Context& ctx, const int* begdat, int isp, int numrxy, int numcxy,
            int begcol, int endcol, int smpday, double* xy, const bool* begrgm,
            bool td1c) {
    using namespace prm;
    constexpr double ZERO = 0.0;

    static const char DAYDIC[] = "montuewedthufrisat";
    static const int dayptr[7] = {1, 4, 7, 10, 13, 16, 19};
    constexpr int PDAY = 6;

    // DATA tables (Fortran column-major fill).
    static const int fouryr[4] = {0, 2, 3, 4};
    static const int lnomo[12][2] = {{31, 31}, {28, 29}, {31, 31}, {30, 30},
                                     {31, 31}, {30, 30}, {31, 31}, {31, 31},
                                     {30, 30}, {31, 31}, {30, 30}, {31, 31}};
    static const int fdomo[12][2] = {{0, 0}, {3, 3}, {3, 4}, {6, 0}, {1, 2},
                                     {4, 5}, {6, 0}, {2, 3}, {5, 6}, {0, 1},
                                     {3, 4}, {5, 6}};
    // ndywkm(0:6, 28:31)
    static const int ndywkm[4][7] = {{4, 4, 4, 4, 4, 4, 4},
                                     {5, 4, 4, 4, 4, 4, 4},
                                     {5, 5, 4, 4, 4, 4, 4},
                                     {5, 5, 5, 4, 4, 4, 4}};
    static const int lnoqtr[4][2] = {{90, 91}, {91, 91}, {92, 92}, {92, 92}};
    static const int fdoqtr[4][2] = {{0, 0}, {6, 0}, {6, 0}, {0, 1}};
    // ndywkq(0:6, 90:92)
    static const int ndywkq[3][7] = {{13, 13, 13, 13, 13, 13, 12},
                                     {13, 13, 13, 13, 13, 13, 13},
                                     {14, 13, 13, 13, 13, 13, 13}};
    static const double stckwt[4] = {-0.6, -0.2, 0.2, 0.6};

    bool ltdstk = (smpday > 0);

    int predat[2];
    addate(begdat, isp, -1, predat);
    int precol = begcol - 1;

    auto XY = [&](int icol, int irow) -> double& {
        return xy[static_cast<std::size_t>((irow - 1) * numcxy + (icol - 1))];
    };

    for (int irow = 1; irow <= numrxy; ++irow) {
        if (begrgm[irow - 1]) {
            int idate[2];
            addate(predat, isp, irow, idate);
            int year = idate[YR - 1];
            int period = idate[MO - 1];
            // Day-of-week of Jan 1 (Sun=0 ... Sat=6).
            int sdoyr = 5 * (year / 4 - 441) + fouryr[year % 4];
            int cendsp = (year - 1601) / 100;
            cendsp = cendsp - cendsp / 4 - 1;
            sdoyr = sdoyr - cendsp;
            sdoyr = sdoyr % 7;
            int lpyr;
            if ((year % 100 != 0 && year % 4 == 0) || year % 400 == 0) lpyr = 2;
            else lpyr = 1;

            int td[7] = {0, 0, 0, 0, 0, 0, 0};
            if (isp == 12) {
                int sdomo = (sdoyr + fdomo[period - 1][lpyr - 1]) % 7;
                int ndomo = lnomo[period - 1][lpyr - 1];
                if (ltdstk) {
                    int dyosmp;
                    if (smpday > ndomo) dyosmp = (sdomo + ndomo - 1) % 7;
                    else dyosmp = (sdomo + smpday - 1) % 7;
                    for (int iday = 0; iday <= 6; ++iday) td[iday] = 0;
                    td[dyosmp] = 1;
                } else {
                    for (int i = 0; i <= 6; ++i) {
                        int iday = (i + sdomo) % 7;
                        td[iday] = ndywkm[ndomo - 28][i];
                    }
                }
            } else if (isp == 4) {
                int sdoqtr = (sdoyr + fdoqtr[period - 1][lpyr - 1]) % 7;
                int ndoqtr = lnoqtr[period - 1][lpyr - 1];
                for (int i = 0; i <= 6; ++i) {
                    int iday = (i + sdoqtr) % 7;
                    td[iday] = ndywkq[ndoqtr - 90][i];
                }
            }

            if (td1c) {
                if (ltdstk) {
                    // Stock 1-coef trading day regressor.
                    XY(begcol, irow) = static_cast<double>(td[5] - td[0]);
                    for (int iday = 1; iday <= 4; ++iday) {
                        XY(begcol, irow) =
                            (stckwt[iday - 1] * static_cast<double>(td[iday] - td[0])) +
                            XY(begcol, irow);
                    }
                } else {
                    XY(begcol, irow) =
                        ZERO - (static_cast<double>(td[0] + td[6]) * 2.5);
                    for (int iday = 1; iday <= 5; ++iday)
                        XY(begcol, irow) = XY(begcol, irow) +
                                           static_cast<double>(td[iday]);
                }
            } else {
                for (int icol = begcol; icol <= endcol; ++icol) {
                    int iday;
                    if ((endcol - begcol + 1) < 6) {
                        std::string str;
                        int nchr;
                        getstr(ctx, ctx.model.colttl.data(), ctx.model.colptr.data(),
                               ctx.model.ncoltl, icol, str, nchr);
                        if (ctx.error.lfatal) return;
                        iday = strinx(false, DAYDIC, dayptr, 1, PDAY,
                                      std::string_view(str).substr(0, 3));
                    } else {
                        iday = icol - begcol + 1;
                    }
                    td[iday] = td[iday] - td[0];
                    XY(icol, irow) = static_cast<double>(td[iday]);
                }
            }
        } else {
            for (int iday = 1; iday <= endcol - begcol + 1; ++iday)
                XY(precol + iday, irow) = ZERO;
        }
    }
}

}  // namespace x13
