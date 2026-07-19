// strvec.cpp -- packed string-vector / pointer-vector machinery used by the
// regression and ARIMA model structure builders (M2 regression-matrix chunk):
// getstr.f, insstr.f, delstr.f, putstr.f (CHARACTER-buffer overload), copy.f,
// copylg.f, insdbl.f, insint.f, inslg.f.
//
// Conventions: chrvec is a fixed-length CHARACTER buffer (fstring<N>::data());
// all positions are Fortran 1-based; ptrvec[k] == Ptrvec(k) with Ptrvec(0)=1.
// Numeric vec pointers satisfy vec[0] == V(1).
#include "specparse/specparse.hpp"

namespace x13 {

// getstr.f  (the CHARACTER target-length overflow check cannot trigger with an
// std::string target; the index-range check is preserved).
void getstr(X13Context& ctx, const char* chrvec, const int* ptrvec, int nstr,
            int istr, std::string& str, int& nchr) {
    if (istr > nstr || istr < 1) {
        writln(ctx, "Index out of range vector", stdio::STDERR, ctx.units.mt2, true);
        abend(ctx);
        return;
    }
    eltlen(ctx, istr, ptrvec, nstr, nchr);
    if (ctx.error.lfatal) return;
    int begstr = ptrvec[istr - 1];
    if (nchr > 0)
        str.assign(chrvec + (begstr - 1), static_cast<std::size_t>(nchr));
    else
        str.clear();
}

// putstr.f (CHARACTER-buffer overload; the std::string overload lives in
// readers_val.cpp).
void putstr(X13Context& ctx, std::string_view str, int pstr, char* chrvec,
            int chrvec_len, int* ptrvec, int& nstr) {
    insptr(ctx, true, static_cast<int>(str.size()), nstr + 1, pstr,
           chrvec_len, ptrvec, nstr);
    if (!ctx.error.lfatal) {
        int b = ptrvec[nstr - 1];
        int e = ptrvec[nstr] - 1;
        for (int i = b; i <= e; ++i)
            chrvec[i - 1] = str[static_cast<std::size_t>(i - b)];
    }
}

// insstr.f
void insstr(X13Context& ctx, std::string_view str, int istr, int pstr,
            char* chrvec, int chrvec_len, int* ptrvec, int& nstr) {
    insptr(ctx, true, static_cast<int>(str.size()), istr, pstr, chrvec_len,
           ptrvec, nstr);
    if (ctx.error.lfatal) return;
    // Copy from last character to the first (the substring is pushed down and
    // may copy over itself).
    int begchr = ptrvec[istr];
    int endchr = ptrvec[nstr] - 1;
    int nwmold = ptrvec[istr] - ptrvec[istr - 1];
    for (int ichr = endchr; ichr >= begchr; --ichr)
        chrvec[ichr - 1] = chrvec[ichr - nwmold - 1];
    // Now there is room to insert the new substring.
    for (int i = ptrvec[istr - 1]; i <= ptrvec[istr] - 1; ++i)
        chrvec[i - 1] = str[static_cast<std::size_t>(i - ptrvec[istr - 1])];
}

// delstr.f
void delstr(X13Context& ctx, int istr, char* chrvec, int* ptrvec, int& nstr,
            int nlim) {
    (void)nlim;
    if (istr > nstr || istr < 1) {
        writln(ctx, "Index out of range vector", stdio::STDERR, ctx.units.mt2, true);
        abend(ctx);
        return;
    }
    int oldbeg = ptrvec[istr];
    int newbeg = ptrvec[istr - 1];
    int nrest = ptrvec[nstr] - oldbeg - 1;
    if (nrest >= 0) {
        for (int k = 0; k <= nrest; ++k)
            chrvec[newbeg - 1 + k] = chrvec[oldbeg - 1 + k];
    }
    int nchr;
    eltlen(ctx, istr, ptrvec, nstr, nchr);
    if (ctx.error.lfatal) return;
    for (int i = istr; i <= nstr - 1; ++i) ptrvec[i] = ptrvec[i + 1] - nchr;
    nstr = nstr - 1;
}

// copy.f  (same-index copy; Inc<0 iterates backwards for overlap safety)
void copy(const double* invec, int n, int inc, double* outvec) {
    int begelt, endelt;
    if (inc > 0) { begelt = 1; endelt = n; }
    else         { begelt = n; endelt = 1; }
    for (int i = begelt; (inc > 0) ? (i <= endelt) : (i >= endelt); i += inc)
        outvec[i - 1] = invec[i - 1];
}

// copylg.f
void copylg(const bool* invec, int n, int inc, bool* outvec) {
    int begelt, endelt;
    if (inc > 0) { begelt = 1; endelt = n; }
    else         { begelt = n; endelt = 1; }
    for (int i = begelt; (inc > 0) ? (i <= endelt) : (i >= endelt); i += inc)
        outvec[i - 1] = invec[i - 1];
}

// insdbl.f
void insdbl(X13Context& ctx, const double* subvec, int ielt, const int* ptrvec,
            int nelt, double* vec) {
    int nielt;
    eltlen(ctx, ielt, ptrvec, nelt, nielt);
    if (ctx.error.lfatal) return;
    int nrest = ptrvec[nelt] - ptrvec[ielt];
    copy(vec + (ptrvec[ielt - 1] - 1), nrest, -1, vec + (ptrvec[ielt] - 1));
    copy(subvec, nielt, 1, vec + (ptrvec[ielt - 1] - 1));
}

// insint.f
void insint(X13Context& ctx, const int* subvec, int ielt, const int* ptrvec,
            int nelt, int* vec) {
    int nielt;
    eltlen(ctx, ielt, ptrvec, nelt, nielt);
    if (ctx.error.lfatal) return;
    int nrest = ptrvec[nelt] - ptrvec[ielt];
    cpyint(vec + (ptrvec[ielt - 1] - 1), nrest, -1, vec + (ptrvec[ielt] - 1));
    cpyint(subvec, nielt, 1, vec + (ptrvec[ielt - 1] - 1));
}

// inslg.f
void inslg(X13Context& ctx, const bool* subvec, int ielt, const int* ptrvec,
           int nelt, bool* vec) {
    int nielt;
    eltlen(ctx, ielt, ptrvec, nelt, nielt);
    if (ctx.error.lfatal) return;
    int nrest = ptrvec[nelt] - ptrvec[ielt];
    copylg(vec + (ptrvec[ielt - 1] - 1), nrest, -1, vec + (ptrvec[ielt] - 1));
    copylg(subvec, nielt, 1, vec + (ptrvec[ielt - 1] - 1));
}

}  // namespace x13
