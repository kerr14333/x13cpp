// specparse.hpp -- declarations for the X-13ARIMA-SEATS spec-file parser
// subsystem (M1). One free function per ported Fortran routine; every routine
// that touches ex-COMMON state takes X13Context& ctx first. Trivial, COMMON-free
// Fortran intrinsics / init helpers (indx, cmpstr, strinx, setint, ...) are
// provided inline here rather than as separate translation units.
#ifndef X13_SPECPARSE_SPECPARSE_HPP
#define X13_SPECPARSE_SPECPARSE_HPP

#include <string>
#include <string_view>
#include <cstddef>

#include "common/x13context.hpp"
#include "specparse/lexstate.hpp"

namespace x13 {

// stdio.i parameters used by the parser.
namespace stdio {
inline constexpr int STDERR = 6;
inline constexpr int STDIN = 5;
inline constexpr int STDOUT = 6;
inline constexpr int PFILCR = 512;
inline constexpr char VERNUM[] = "1.1";
inline constexpr char PRGNAM[] = "X-13ARIMA-SEATS";
inline constexpr char DOCNAM[] = "Reference Manual";
inline constexpr char LIMSEC[] = "Section 2.7";
} // namespace stdio

// --------------------------------------------------------------------------
// Fortran intrinsics / tiny COMMON-free helpers (inline).
// --------------------------------------------------------------------------

// INDEX-style scan: 1-based position of Chr in Str, 0 if not found (indx.f).
inline int indx(std::string_view str, char chr) {
    for (std::size_t i = 0; i < str.size(); ++i)
        if (str[i] == chr) return static_cast<int>(i + 1);
    return 0;
}

// nblank.f: index of last non-blank char (0 if all blank). 1-based.
inline int nblank(std::string_view str) {
    for (std::size_t i = str.size(); i > 0; --i)
        if (str[i - 1] != ' ') return static_cast<int>(i);
    return 0;
}

// clsgrp.f: closing char code for an opening group char.
inline int clsgrp(int opnchr) {
    if (opnchr == 40) return 41;      // ( -> )
    if (opnchr == 47) return 47;      // / -> /
    if (opnchr == 91) return 93;      // [ -> ]
    if (opnchr == 123) return 125;    // { -> }
    return -1;
}

// map.f specialized to case folding: UCASE<->LCASE both length 26.
inline char map_case(std::string_view frcset, std::string_view tocset, char c) {
    int m = indx(frcset, c);
    return (m > 0) ? tocset[static_cast<std::size_t>(m - 1)] : c;
}

// cmpstr.f: compare two strings by token type (NAME = case-insensitive, else exact).
bool cmpstr(int toktyp, std::string_view str1, std::string_view str2);

// strinx.f: dictionary index (1-based) of Str in Chrvec split by Ptrvec, else 0.
int strinx(bool chksub, std::string_view chrvec, const int* ptrvec,
           int begstr, int endstr, std::string_view str);

// ctoi.f / ctod.f: parse integer / double from Str starting at 1-based Ipos,
// advancing Ipos past the number (Ipos unchanged if none present).
int ctoi(std::string_view str, int& ipos);
double ctod(std::string_view str, int& ipos);

// isdate.f / addate.f / dfdate.f / chkcvr.f : date arithmetic (dates are int[2]
// laid out as d[YR], d[MO] with YR=1, MO=2 -> here d[0]=year, d[1]=period).
bool isdate(const int* dat, int sp);
void addate(const int* indate, int sp, int b10, int* outdat);
void dfdate(const int* datea, const int* dateb, int sp, int& diff);
bool chkcvr(const int* begsrs, int nobs, const int* begspn, int nspobs, int sp);

// Vector init helpers (setint.f/setdp.f/setlg.f/setchr.f/cpyint.f). Inlined.
inline void setint(int val, int n, int* vec) { for (int i = 0; i < n; ++i) vec[i] = val; }
inline void setdp(double val, int n, double* vec) { for (int i = 0; i < n; ++i) vec[i] = val; }
inline void setlg(bool val, int n, bool* vec) { for (int i = 0; i < n; ++i) vec[i] = val; }
inline void cpyint(const int* invec, int n, int inc, int* outvec) {
    if (inc > 0) for (int i = 0; i < n; ++i) outvec[i] = invec[i];
    else for (int i = n - 1; i >= 0; --i) outvec[i] = invec[i];
}

// itoc.f: write Inum into Str at 1-based Ipos, advancing Ipos past it.
void itoc(X13Context& ctx, int inum, std::string& str, int& ipos);

// --------------------------------------------------------------------------
// Error / output.
// --------------------------------------------------------------------------
void abend(X13Context& ctx);
void errhdr(X13Context& ctx);
void writln(X13Context& ctx, std::string_view oline, int flhdnl, int flhdn2, bool lblnk);
void inpter(X13Context& ctx, int errtyp, const int* ptr, std::string_view errmsg);

// --------------------------------------------------------------------------
// Input buffer + lexer.
// --------------------------------------------------------------------------
bool rngbuf(X13Context& ctx, int cmd, int& linno, std::string& lin, int& linln);
void intinp(X13Context& ctx, int instr);
char getchr(X13Context& ctx, char& nxtchr);
void putbak(X13Context& ctx, char lstchr);
int whitsp(X13Context& ctx);
void lex(X13Context& ctx);
void qtoken(X13Context& ctx);
bool qname(X13Context& ctx, std::string& astrng, int& nchr);
bool qquote(X13Context& ctx, std::string& astrng, int& nchr);
bool qcmmnt(X13Context& ctx, std::string& str, int& nchr);
bool qintgr(X13Context& ctx, std::string& str, int off, int& nchr);
bool qdoble(X13Context& ctx, std::string& str, int& nchr, bool& alsoin);

// --------------------------------------------------------------------------
// Dictionary / argument machinery.
// --------------------------------------------------------------------------
void gtdcnm(X13Context& ctx, std::string_view args, const int* argptr, int nargs,
            int& argidx, bool& argok);
bool gtarg(X13Context& ctx, std::string_view args, const int* argptr, int nargs,
           int& argidx, int* arglog, bool& inptok);
bool getfcn(X13Context& ctx, std::string_view fcns, const int* fcnptr, int nfcns,
            int& fcnidx, int* fcnlog, bool& inptok);
void skparg(X13Context& ctx);
void skpfcn(X13Context& ctx, std::string_view fname, int nfn);
void skplst(X13Context& ctx, int clsgtp);
void skparm(X13Context& ctx, bool lauto);

// --------------------------------------------------------------------------
// Typed value readers.
// --------------------------------------------------------------------------
bool getint(X13Context& ctx, int& tmp);
bool getdbl(X13Context& ctx, double& tmp);
void eltlen(X13Context& ctx, int istr, const int* ptrvec, int nstr, int& length);
void intlst(int pelt, int* ptrvec, int& nstr);

void getivc(X13Context& ctx, int grpchr, bool flgnul, int pelt, int* avec,
            int& nelt, bool& locok, bool& inptok);
void gtdpvc(X13Context& ctx, int grpchr, bool flgnul, int pelt, double* avec,
            int& nelt, bool& locok, bool& inptok);
void gtdtvc(X13Context& ctx, bool& havesp, int& sp, int grpchr, bool flgnul,
            int pdt, int* avec, int& nelt, bool& locok, bool& inptok);
void gtnmvc(X13Context& ctx, int grpchr, bool flgnul, int pelt, std::string& chrvec,
            int* ptrvec, int& nelt, int maxchr, bool& locok, bool& inptok);
void getttl(X13Context& ctx, int grpchr, bool flgnul, int pelt, std::string& chrvec,
            int* ptrvec, int& nelt, bool& locok, bool& inptok);
void gtdcvc(X13Context& ctx, int grpchr, bool flgnul, int pelt, std::string_view dic,
            const int* dicptr, int ndic, std::string_view errmsg, int* avec,
            int& nelt, bool& locok, bool& inptok);
// getvec.f : general string-list reader used by getprt/getsav/variable lists.
void getvec(X13Context& ctx, int grpchr, bool flgnul, int pelt, std::string& chrvec,
            int* ptrvec, int& nelt, int maxchr, bool& locok, bool& inptok);

// print/save table selection (getprt.f/getsav.f) -- M1 consumes tokens; the
// table-dictionary application is stubbed (see notes).
void getprt(X13Context& ctx, int lspsrs, int nspsrs, bool& locok);
void getsav(X13Context& ctx, int lspsrs, int nspsrs, bool& locok);
void getsvl(X13Context& ctx, int lsvsrs, int nsvsrs, bool& locok);

// --------------------------------------------------------------------------
// Convenience accessors for the current token text.
// --------------------------------------------------------------------------
inline std::string cur_tok(const X13Context& ctx) {
    return ctx.lex.nxttok.substr(0, static_cast<std::size_t>(ctx.lex.nxtkln));
}

} // namespace x13

#endif // X13_SPECPARSE_SPECPARSE_HPP
