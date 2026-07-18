// dtoc.cpp -- dtoc.f: format a double using the run's fixed save format.
#include "tables/tables.hpp"

#include <cmath>
#include <string>

#include "specparse/specparse.hpp"   // abend
#include "x13/fformat.hpp"

namespace x13 {

namespace {
// Trim trailing blanks from a fixed-width format string, e.g. "(sp,e22.15) " .
std::string trim_fmt(const std::string& s) {
    std::size_t e = s.find_last_not_of(' ');
    return (e == std::string::npos) ? std::string() : s.substr(0, e + 1);
}
}  // namespace

// dtoc.f
void dtoc(X13Context& ctx, double dnum, std::string& str, int& ipos) {
    int nleft = static_cast<int>(str.size()) - (ipos - 1);
    double d10 = (dnum == 0.0) ? 0.0 : std::log10(std::fabs(dnum));
    int svsize = ctx.savcmn.svsize;
    std::string fmt = trim_fmt(ctx.savcmn.svfmt.str());

    if (svsize > nleft) {
        // Cannot write value in the remaining space -- fatal (dtoc.f).
        abend(ctx);
        return;
    }
    std::string val = (d10 > -100.0) ? fwrite_fmt(fmt, dnum) : fwrite_fmt(fmt, 0.0);
    for (int k = 0; k < svsize; ++k)
        str[static_cast<std::size_t>(ipos - 1 + k)] =
            (k < static_cast<int>(val.size())) ? val[static_cast<std::size_t>(k)] : ' ';
    ipos += svsize;
}

}  // namespace x13
