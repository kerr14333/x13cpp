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
// Packed string-vector / pointer-vector machinery (M2 regression chunk).
// A "string vector" is a CHARACTER buffer Chrvec plus Ptrvec(0:N) of 1-based
// begin positions; element i occupies Chrvec(Ptrvec(i-1):Ptrvec(i)-1).
// Numeric companions (B, Rgvrtp, ...) use the same Ptrvec via ins*/eltlen.
// ptrvec pointers use ptrvec[k] == Ptrvec(k) (pass farray1lb<..,0,..>::data()).
// --------------------------------------------------------------------------

// insptr.f: open up (or widen) element Ielt of Ptrvec by Niunit units.
void insptr(X13Context& ctx, bool addcat, int niunit, int ielt, int pelt,
            int nunit, int* ptrvec, int& nelt);

// putstr.f: append Str as element Nstr+1 (std::string and CHARACTER buffers).
void putstr(X13Context& ctx, std::string_view str, int pstr, std::string& chrvec,
            int* ptrvec, int& nstr);
void putstr(X13Context& ctx, std::string_view str, int pstr, char* chrvec,
            int chrvec_len, int* ptrvec, int& nstr);

// getstr.f: fetch element Istr into Str (exact length, no padding).
void getstr(X13Context& ctx, const char* chrvec, const int* ptrvec, int nstr,
            int istr, std::string& str, int& nchr);

// insstr.f / delstr.f: insert / delete element Istr of a CHARACTER buffer.
void insstr(X13Context& ctx, std::string_view str, int istr, int pstr,
            char* chrvec, int chrvec_len, int* ptrvec, int& nstr);
void delstr(X13Context& ctx, int istr, char* chrvec, int* ptrvec, int& nstr,
            int nlim);

// copy.f / copylg.f: same-index vector copies with Inc controlling direction
// (overlap-safe shifts). Pointers are to the Fortran 1-position (vec[0]==V(1)).
void copy(const double* invec, int n, int inc, double* outvec);
void copylg(const bool* invec, int n, int inc, bool* outvec);

// insdbl.f / insint.f / inslg.f: insert Subvec as (pre-widened) element Ielt of
// the Ptrvec-partitioned vector Vec.
void insdbl(X13Context& ctx, const double* subvec, int ielt, const int* ptrvec,
            int nelt, double* vec);
void insint(X13Context& ctx, const int* subvec, int ielt, const int* ptrvec,
            int nelt, int* vec);
void inslg(X13Context& ctx, const bool* subvec, int ielt, const int* ptrvec,
           int nelt, bool* vec);

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
// --------------------------------------------------------------------------
// Spec readers (gtinpt dispatch targets). All are M1 spec-parser ports; deep
// estimation/X11/SEATS processing beyond argument parsing is deferred.
// --------------------------------------------------------------------------
void getsrs(X13Context& ctx, bool& havsrs, bool& havesp, bool& lagr, bool ldata,
            std::string& dtafil, bool& inptok);
void gtinpt(X13Context& ctx, bool& lx11, bool& lseats, bool& lmodel, bool& inptok);

// mdlfin: finalize the parsed ARMA model for estimation (gtinpt.f:220 block).
// Derives, from the operator max-lags set by getmdl and the exact-ARMA switches
// (Lextar/Lextma, from estimate{}), the flags the estimation engine reads:
//   Lar = Lextar & Mxarlg>0,  Lma = Lextma & Mxmalg>0
//   Lextar:  Nintvl = Mxdflg,          Nextvl = Mxarlg + Mxmalg
//   else:    Nintvl = Mxdflg + Mxarlg, Nextvl = Lextma ? Mxmalg : 0
// Nintvl is the total differencing order (drops effective observations),
// Nextvl the extra-observation floor rgarma checks against. Pure int/bool.
void mdlfin(X13Context& ctx);

// Simple spec readers: each consumes its arguments token-faithfully and
// captures key settings. Signature reduced to (ctx, inptok).
void gt_transform(X13Context& ctx, bool& inptok);   // transform{} (getadj)
void gt_regression(X13Context& ctx, bool havsrs, bool havesp,
                   bool& havtd, bool& inptok);      // regression{} (getreg)
void gt_arima(X13Context& ctx, bool& inptok);        // arima{} (gtarma+getmdl)

// --------------------------------------------------------------------------
// arima{ model = ... } structure builders (M2 regression-matrix chunk).
// --------------------------------------------------------------------------
void polyml(const double* polya, const int* alag, int na, const double* polyb,
            const int* blag, int nb, int pc, double* polyc, int* clag, int& nc);
void insort(double tcoef, int lagt, int& ncoef, double* coef, int* lag);
void inbtwn(double tcoef, int lagt, int in, int& ncoef, double* coef, int* lag);
void intsrt(int nr, int* vecx);
void iscrfn(int oprn, int scr, const int* avec, int nelt, int pc, int* cvec);
void mkoprt(X13Context& ctx, int optype, int period, int sp, std::string& oprnam,
            int& noprcr);
void maxlag(const int* arimal, const int* opr, int begopr, int endopr, int& mxlag);
void getopr(X13Context& ctx, int optype, double* coef, int* lag, bool* fix,
            int& ncoef, int& nd, int& naimcf, bool& locok, bool& inptok);
void insopr(X13Context& ctx, int optype, const double* coef, const int* lag,
            const bool* fix, int ncoef, int facsp, std::string_view ioprtl,
            bool& locok, bool& inptok);
void getmdl(X13Context& ctx, bool& locok, bool& inptok, bool lauto);
void mdlfix(X13Context& ctx);

// --------------------------------------------------------------------------
// regression{ variables = ... } structure builders (getreg.f subtree).
// --------------------------------------------------------------------------
void adrgef(X13Context& ctx, double initvl, std::string_view effttl,
            std::string_view igrptl, int vartyp, bool varfix, bool userin);
void rdregm(X13Context& ctx, std::string_view rgmttl, const int* begspn, int sp,
            int& zeroz, int& rgmidx, bool& locok);
void adpdrg(X13Context& ctx, const int* begsrs, const int* endmdl, int nobs,
            bool havsrs, bool havesp, std::string rgname, int nrgchr,
            bool x11reg, bool& havtd, bool& havhol, bool& havln, bool& havlp,
            bool& locok, bool& inptok);
void gtpdrg(X13Context& ctx, const int* begsrs, const int* endmdl, int nobs,
            bool havsrs, bool havesp, bool x11reg, bool& havtd, bool& havhol,
            bool& havln, bool& havlp, bool& locok, bool& inptok);
void rmlnvr(X13Context& ctx, int& priadj, int kfulsm, int nspobs);
void dlrgef(X13Context& ctx, int begcol, int nrxy, int ndelc);
// ctodat.f (shared with the date readers; exported for rdregm).
void ctodat(std::string_view str, int sp, int& ipos, int* idate, bool& locok);
void gt_automdl(X13Context& ctx, bool& inptok);      // automdl{} (gtauto)
void gt_estimate(X13Context& ctx, bool& inptok);     // estimate{} (gtestm)
void gt_outlier(X13Context& ctx, bool& inptok);      // outlier{} (gtotlr)
void gt_forecast(X13Context& ctx, bool& inptok);     // forecast{} (gtfcst)
void gt_x11(X13Context& ctx, bool& inptok);          // x11{} (getx11)
void gt_seats(X13Context& ctx, bool& inptok);        // seats{} (gtseat)
void gt_force(X13Context& ctx, bool& inptok);        // force{} (getfrc)
void gt_slidingspans(X13Context& ctx, bool& havesp, bool& inptok); // slidingspans{} (getssp)
void gt_history(X13Context& ctx, bool& havesp, bool& inptok);      // history{} (gtrvst)
void gt_check(X13Context& ctx, bool& inptok);        // check{} (getchk)
void gt_identify(X13Context& ctx, bool& inptok);     // identify{} (getid)
void gt_composite(X13Context& ctx, bool& havsrs, bool& lagr, bool& inptok); // composite{}
void gt_metadata(X13Context& ctx, bool& inptok);     // metadata{} (gtmtdt)
void gt_spectrum(X13Context& ctx, bool& inptok);     // spectrum{} (gtspec)
void gt_pickmdl(X13Context& ctx, bool& inptok);      // pickmdl{} (gtautx)
void gt_x11regression(X13Context& ctx, bool havsrs, bool havesp, bool& inptok);// x11regression{} (gtxreg)
void gt_generic(X13Context& ctx, std::string_view argdic, const int* argptr,
                int narg, bool& inptok);             // shared arg-consumer

inline std::string cur_tok(const X13Context& ctx) {
    return ctx.lex.nxttok.substr(0, static_cast<std::size_t>(ctx.lex.nxtkln));
}

// M1 library entry point (core/src/driver/parse_spec.cpp).
bool parse_spec(X13Context& ctx, const std::string& spec_text,
                const std::string& infile_name);

// M2 pre-model phase (core/src/driver/run_pre_model.cpp): parse, then run the
// reachable pre-model table/save output (currently table a1 -- the original
// series over the analyzed span). `base` is the spec base name (Serno/Cursrs).
// With `estimate` true (the M3 path), the built regARIMA model is estimated in
// place after the pre-model saves (rgarma; results land in ctx.mdldat); the
// default false keeps the M2 save-only behavior the M2 parity gate depends on.
bool run_m2(X13Context& ctx, const std::string& spec_text, const std::string& base,
            bool estimate = false);

// Post-parse body of run_m2 (pre-model saves + optional estimate/forecast) on an
// already-parsed context. out_trnsrs/out_nobspf, when non-null, return the clean
// transformed series and its length for the X-11 extend stage. See run_x11.
bool run_m2_after_parse(X13Context& ctx, const std::string& base, bool estimate,
                        std::vector<double>* out_trnsrs, int* out_nobspf);

// M5 X-11 phase (core/src/driver/run_x11.cpp): parse, then assemble the classic
// X-11 decomposition spine (setxpt -> x11int -> x11pt1 -> x11pt2) producing the
// B/C/D tables (B1..D7) in the ctx x11srs arrays. Wired for the no-model direct-
// X11 path (airline_x11-default) so far; a spec carrying a regARIMA model fatals
// cleanly until the estimate/forecast/extend/adjreg glue lands.
bool run_x11(X13Context& ctx, const std::string& spec_text, const std::string& base);

// M6(scoping) SEATS phase (core/src/driver/run_seats.cpp): parse, estimate the
// regARIMA model (SEATS is always model-based -- no direct-SEATS path exists
// in the oracle), then dispatch to the SEATS decomposition. The decomposition
// itself (root allocation/canonical split/WK-filter signal extraction) is not
// yet ported (see tools/seats_scope.md); this currently always returns false
// with a "not yet ported" fatal once the model is in hand, giving the parity
// harness (tools/x13run_seats.cpp) a stable, xfailed gate to iterate against.
bool run_seats(X13Context& ctx, const std::string& spec_text, const std::string& base);

} // namespace x13

#endif // X13_SPECPARSE_SPECPARSE_HPP
