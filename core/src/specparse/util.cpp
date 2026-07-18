// util.cpp -- COMMON-free Fortran utility routines for the spec parser:
// cmpstr.f, strinx.f, ctoi.f, ctod.f, isdate.f, addate.f, dfdate.f, chkcvr.f,
// itoc.f. Grouped into one translation unit (all tiny, no shared state).
#include "specparse/specparse.hpp"

#include <cmath>

namespace x13 {

using namespace lexprm;

// cmpstr.f
bool cmpstr(int toktyp, std::string_view str1, std::string_view str2) {
    if (toktyp == QUOTE) return str1 == str2;
    if (toktyp == NAME) {
        if (str1.size() != str2.size()) return false;
        static const std::string_view UC = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static const std::string_view LC = "abcdefghijklmnopqrstuvwxyz";
        for (std::size_t i = 0; i < str1.size(); ++i) {
            char c1 = map_case(UC, LC, str1[i]);
            char c2 = map_case(UC, LC, str2[i]);
            if (c1 != c2) return false;
        }
        return true;
    }
    return str1 == str2;
}

// strinx.f
int strinx(bool chksub, std::string_view chrvec, const int* ptrvec,
           int begstr, int endstr, std::string_view str) {
    int nchrm1 = static_cast<int>(str.size()) - 1;
    for (int i = begstr; i <= endstr; ++i) {
        int begchr = ptrvec[i - 1];
        int endchr = ptrvec[i] - 1;
        if (chksub) endchr = std::min(endchr, begchr + nchrm1);
        if (endchr >= begchr) {
            std::string_view sub = chrvec.substr(static_cast<std::size_t>(begchr - 1),
                                                 static_cast<std::size_t>(endchr - begchr + 1));
            if (cmpstr(NAME, str, sub)) return i;
        }
    }
    return 0;
}

// ctoi.f
int ctoi(std::string_view str, int& ipos) {
    int nchr = static_cast<int>(str.size());
    int lstpt = ipos;
    bool havint = false;
    int insign = 1;
    auto at = [&](int p) -> char { return str[static_cast<std::size_t>(p - 1)]; };
    if (at(ipos) == '+' || at(ipos) == '-') {
        if (at(ipos) == '-') insign = -1;
        ++ipos;
    }
    int val = 0;
    for (; ipos <= nchr; ++ipos) {
        int digit = indx("0123456789", at(ipos)) - 1;
        if (digit == -1) break;
        havint = true;
        val = 10 * val + digit;
    }
    val = insign * val;
    if (!havint) ipos = lstpt;
    return val;
}

// ctod.f
double ctod(std::string_view str, int& ipos) {
    int nchr = static_cast<int>(str.size());
    int lstpt = ipos;
    bool havdbl = false;
    double dpsign = 1.0;
    auto at = [&](int p) -> char { return str[static_cast<std::size_t>(p - 1)]; };
    if (at(ipos) == '+' || at(ipos) == '-') {
        if (at(ipos) == '-') dpsign = -1.0;
        ++ipos;
    }
    double val = 0.0;
    for (; ipos <= nchr; ++ipos) {
        int digit = indx("0123456789", at(ipos)) - 1;
        if (digit == -1) goto after_int;
        val = 10.0 * val + static_cast<double>(digit);
        havdbl = true;
    }
    ipos = nchr + 1;
after_int:
    if (ipos <= nchr && at(ipos) == '.') {
        ++ipos;
        double scl = 1.0;
        for (; ipos <= nchr; ++ipos) {
            scl = scl * 10.0;
            int digit = indx("0123456789", at(ipos)) - 1;
            if (digit == -1) goto after_frac;
            val = val + static_cast<double>(digit) / scl;
            havdbl = true;
        }
        ipos = nchr + 1;
    }
after_frac:
    val = dpsign * val;
    if (havdbl && ipos < nchr) {
        if (indx("eEdD^", at(ipos)) > 0) {
            int exppos = ipos;
            ++ipos;
            int expint = ctoi(str, ipos);
            if (ipos == exppos + 1) ipos = exppos;
            else val = val * std::pow(10.0, static_cast<double>(expint));
        }
    }
    if (!havdbl) { val = 0.0; ipos = lstpt; }
    return val;
}

// isdate.f  (dat[0]=year, dat[1]=period)
bool isdate(const int* dat, int sp) {
    if ((sp > 1 && (dat[1] < 1 || dat[1] > sp)) || dat[0] < 0) return false;
    return true;
}

// addate.f
void addate(const int* indate, int sp, int b10, int* outdat) {
    if (sp == 1) {
        outdat[0] = indate[0] + b10;
        outdat[1] = 0;
        return;
    }
    int iadd = indate[0] * sp + indate[1] + b10;
    outdat[0] = iadd / sp;
    outdat[1] = iadd % sp;
    if (outdat[1] < 0) {
        outdat[0] -= 1;
        outdat[1] = sp + outdat[1];
    } else if (outdat[1] == 0 && outdat[0] == 0) {
        outdat[0] = -1;
        outdat[1] = sp;
    } else if (outdat[1] == 0) {
        outdat[0] -= 1;
        outdat[1] = sp;
    }
}

// dfdate.f
void dfdate(const int* datea, const int* dateb, int sp, int& diff) {
    if (sp > 1) diff = sp * (datea[0] - dateb[0]) + datea[1] - dateb[1];
    else diff = datea[0] - dateb[0];
}

// chkcvr.f
bool chkcvr(const int* begsrs, int nobs, const int* begspn, int nspobs, int sp) {
    int idif;
    dfdate(begspn, begsrs, sp, idif);
    if (idif < 0 || idif + nspobs > nobs || nspobs <= 0) return false;
    return true;
}

// itoc.f  (writes into Str at 1-based Ipos; Str is a std::string long enough)
void itoc(X13Context& ctx, int inum, std::string& str, int& ipos) {
    static const char* digits = "0123456789";
    int nleft = static_cast<int>(str.size()) - (ipos - 1);
    int begchr = ipos;
    if (inum < 0) {
        str[static_cast<std::size_t>(begchr - 1)] = '-';
        ++begchr;
    }
    int intval = std::abs(inum);
    int nchr;
    if (intval == 0) {
        nchr = 1;
    } else {
        double tmp = std::log10(static_cast<double>(intval)) + 1.0;
        nchr = static_cast<int>(tmp) + begchr - ipos;
    }
    if (nchr > nleft) {
        abend(ctx);
        return;
    }
    nchr = ipos - 1 + nchr;
    for (ipos = nchr; ipos >= begchr; --ipos) {
        int d = intval % 10;
        str[static_cast<std::size_t>(ipos - 1)] = digits[d];
        intval /= 10;
    }
    ipos = nchr + 1;
}

} // namespace x13
