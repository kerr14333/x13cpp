// series.cpp -- series{} spec reader (getsrs.f) and its free-format data
// loader (gtfldt.f/gtfree.f/lendp.f, free-format path only). Formatted /
// X-11 / TRAMO / X-12-save readers are deferred (M1 corpus is free-format).
#include "specparse/specparse.hpp"
#include "notset.hpp"
#include "srslen.hpp"

#include <fstream>
#include <sstream>

namespace x13 {

using namespace lexprm;

namespace {
constexpr int YR = 0, MO = 1;   // 0-based date components

// dpeq: Fortran near-equality against DNOTST sentinel.
inline bool is_dnotst(double v) { return v == prm::DNOTST; }
}  // namespace

// gtfldt.f free-format path: read whitespace-separated reals from Datfil.
// Exposed (getreg.f user= file= also loads a free-format regressor matrix).
void gtfldt_free(X13Context& ctx, int plen, const std::string& datfil, int ndfl,
                 double* y, int& nobs, bool& hvfreq, int& freq, bool hvstrt,
                 bool& argok, bool& inptok) {
    argok = true;
    for (int i = 0; i < plen; ++i) y[i] = prm::DNOTST;
    std::string fname = datfil.substr(0, static_cast<std::size_t>(ndfl));
    std::ifstream in(fname);
    if (!in) {
        argok = false;
        inpter(ctx, PERRNP, ctx.lex.pos.data() + 1, "Could not open the data file.");
        nobs = 0;
    } else {
        if (!hvfreq && hvstrt) { freq = 12; hvfreq = true; }
        int i = 0;
        double v;
        while (i < plen && (in >> v)) { y[i] = v; ++i; }
        // A non-numeric token that is not end-of-file is a read error.
        if (!in.eof() && in.fail()) {
            argok = false;
            nobs = 0;
        }
    }
    if (argok) {
        // lendp.f
        nobs = 0;
        for (int lenx = plen; lenx >= 1; --lenx) {
            if (!is_dnotst(y[lenx - 1])) { nobs = lenx; break; }
        }
        if (nobs == 0) {
            argok = false;
        }
    }
    inptok = inptok && argok;
}

// getsrs.f
void getsrs(X13Context& ctx, bool& havsrs, bool& havesp, bool& lagr, bool ldata,
            std::string& dtafil, bool& inptok) {
    LexState& L = ctx.lex;
    constexpr int PARG = 24;
    static const char ARGDIC[] =
        "datastartperiodspantitlefileformatprintsavenameprecisiondecimals"
        "modelspancomptypecompwtmissingcodemissingvalsaveprecisionyr2000"
        "trimzerodivpowerappendfcstappendbcsttype";
    static const int argptr[PARG + 1] = {1, 5, 10, 16, 20, 25, 29, 35, 40, 44, 48,
        57, 65, 74, 82, 88, 99, 109, 122, 128, 136, 144, 154, 164, 168};
    static const char CMPDIC[] = "noneaddsubmultdiv";
    static const int cmpptr[6] = {1, 5, 8, 11, 15, 18};
    static const char YSNDIC[] = "yesno";
    static const int ysnptr[3] = {1, 4, 6};
    static const char ZRODIC[] = "yesspanno";
    static const int zroptr[4] = {1, 4, 8, 10};
    static const char TYPDIC[] = "flowstock";
    static const int typptr[3] = {1, 5, 10};

    int arglog[2 * PARG];
    int& sp = ctx.model.sp;
    double* y = ctx.arima.y.data();
    int& nobs = ctx.arima.nobs;
    int* start = ctx.arima.begsrs.data();
    int* begspn = ctx.mdldat.begspn.data();
    int& nspobs = ctx.mdldat.nspobs;
    int* begmdl = ctx.arima.begmdl.data();
    int* endmdl = ctx.arima.endmdl.data();

    bool locok = true;
    havsrs = false;
    bool havfil = false, havttl = false, havfmt = false, hvstrt = false;
    havesp = false;
    bool hvnam = false, hvmdsp = false;
    int ltrim = 0, nfmtch = 1;
    int spnvec[4], spnmdl[4], endspn[2];
    setint(prm::NOTSET, 4, spnvec);
    setint(prm::NOTSET, 4, spnmdl);
    setint(prm::NOTSET, 2 * PARG, arglog);
    setint(prm::NOTSET, 2, endspn);
    int numdec = 0;

    std::string file(stdio::PFILCR, ' ');
    std::string fmt(stdio::PFILCR, ' ');
    int nflchr = 0;
    if (ldata) {
        file = dtafil;
        nflchr = nblank(dtafil);
        havfil = true;
    }

    std::string srsttl(prm::PSRSCR, ' ');
    std::string srsnam(64, ' ');
    int nttlcr = 0, nser = 0;
    int tmpptr[2];
    int ivec[1];
    double dvec[1];
    int nelt = 0;
    bool argok = false;

    int argidx;
    while (true) {
        if (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
            if (ctx.error.lfatal) return;
            switch (argidx) {
            case 1:  // data
                if (ldata) { inpter(ctx, PERROR, L.errpos.data() + 1,
                        "Cannot use data argument when a data metafile is used."); locok = false; }
                else if (havfil) { inpter(ctx, PERROR, L.errpos.data() + 1,
                        "Use either data or file, not both"); locok = false; }
                gtdpvc(ctx, LPAREN, true, prm::PLEN, y, nobs, argok, locok);
                if (ctx.error.lfatal) return;
                if (argok && nobs > 0) havsrs = true; else nobs = 0;
                break;
            case 2:  // start
                gtdtvc(ctx, havesp, sp, LPAREN, false, 1, start, nelt, argok, locok);
                if (ctx.error.lfatal) return;
                hvstrt = argok && nelt > 0;
                break;
            case 3:  // period
                getivc(ctx, LPAREN, true, 1, ivec, nelt, argok, locok);
                if (ctx.error.lfatal) return;
                if (nelt > 0) {
                    if (!argok) { inpter(ctx, PERROR, L.errpos.data() + 1, "Invalid seasonal period"); locok = false; }
                    else if (ivec[0] > prm::PSP) { inpter(ctx, PERROR, L.errpos.data() + 1, "Seasonal period too large."); locok = false; }
                    else if (havesp && ivec[0] != sp) { inpter(ctx, PERROR, L.errpos.data() + 1, "Assumed seasonal period of 12"); locok = false; }
                    else { havesp = true; sp = ivec[0]; }
                }
                break;
            case 4:  // span
                gtdtvc(ctx, havesp, sp, LPAREN, false, 2, spnvec, nelt, argok, locok);
                if (ctx.error.lfatal) return;
                if (nelt == 1) { inpter(ctx, PERROR, L.errpos.data() + 1,
                        "Need two dates for the span or use a comma as place holder."); locok = false; }
                break;
            case 5:  // title
                getttl(ctx, LPAREN, true, 1, srsttl, tmpptr, nelt, argok, locok);
                if (!ctx.error.lfatal && argok && nelt == 1) { eltlen(ctx, 1, tmpptr, nelt, nttlcr); havttl = true; }
                if (ctx.error.lfatal) return;
                break;
            case 6:  // file
                if (havsrs) { inpter(ctx, PERROR, L.errpos.data() + 1, "Use either data or file, not both"); locok = false; }
                if (ldata) { inpter(ctx, PERROR, L.errpos.data() + 1,
                        "Cannot use file argument when a data metafile is used."); locok = false; }
                gtnmvc(ctx, LPAREN, true, 1, file, tmpptr, nelt, stdio::PFILCR, argok, locok);
                if (ctx.error.lfatal) return;
                if (argok && nelt == 1) { eltlen(ctx, 1, tmpptr, nelt, nflchr); if (ctx.error.lfatal) return; havfil = true; }
                break;
            case 7:  // format
                gtnmvc(ctx, LPAREN, true, 1, fmt, tmpptr, nelt, stdio::PFILCR, argok, locok);
                if (ctx.error.lfatal) return;
                if (argok && nelt == 1) { eltlen(ctx, 1, tmpptr, nelt, nfmtch); if (ctx.error.lfatal) return; havfmt = true; }
                break;
            case 8:  // print
                getprt(ctx, 0, 10, locok);
                break;
            case 9:  // save
                getsav(ctx, 0, 10, locok);
                break;
            case 10:  // name
                gtnmvc(ctx, LPAREN, true, 1, srsnam, tmpptr, nelt, 64, argok, locok);
                if (ctx.error.lfatal) return;
                if (argok) { hvnam = true; eltlen(ctx, 1, tmpptr, nelt, nser); }
                break;
            case 11:  // precision
                getivc(ctx, LPAREN, true, 1, ivec, nelt, argok, locok);
                if (ctx.error.lfatal) return;
                if (argok) {
                    if (ivec[0] < 0 || ivec[0] > 5) { inpter(ctx, PERROR, L.errpos.data() + 1,
                            "Number of input decimals must be between 0 and 5, inclusive"); locok = false; }
                    else numdec = ivec[0];
                }
                break;
            case 12:  // decimals
                getivc(ctx, LPAREN, true, 1, ivec, nelt, argok, locok);
                if (ctx.error.lfatal) return;
                if (argok) {
                    if (ivec[0] < 0 || ivec[0] > 5) { inpter(ctx, PERROR, L.errpos.data() + 1,
                            "Number of output decimals must be between 0 and 5, inclusive"); locok = false; }
                    else ctx.x11opt.kdec = ivec[0];
                }
                break;
            case 13:  // modelspan
                gtdtvc(ctx, havesp, sp, LPAREN, false, 2, spnmdl, nelt, argok, inptok);
                if (ctx.error.lfatal) return;
                if (nelt == 1) { inpter(ctx, PERROR, L.errpos.data() + 1,
                        "Need two dates for the model span or use a comma as place holder."); inptok = false; }
                else if (argok) hvmdsp = true;
                break;
            case 14:  // comptype
                gtdcvc(ctx, LPAREN, true, 1, CMPDIC, cmpptr, 5,
                       "Available composite types are none, add, sub, mult, div.", ivec, nelt, argok, inptok);
                if (ctx.error.lfatal) return;
                if (argok && nelt > 0) { ctx.agr.iag = ivec[0] - 2; if (ctx.agr.iagr == 0 && ctx.agr.iag >= 0) lagr = true; }
                break;
            case 15:  // compwt
                gtdpvc(ctx, LPAREN, true, 1, dvec, nelt, argok, inptok);
                if (ctx.error.lfatal) return;
                if (argok && nelt > 0) {
                    if (dvec[0] <= 0.0) { inpter(ctx, PERROR, L.errpos.data() + 1,
                            "Value of composite weight must be greater than zero."); inptok = false; }
                    else ctx.agr.w = dvec[0];
                }
                break;
            case 16:  // missingcode
                gtdpvc(ctx, LPAREN, true, 1, dvec, nelt, argok, inptok);
                if (ctx.error.lfatal) return;
                ctx.missng.mvcode = dvec[0];
                break;
            case 17:  // missingval
                gtdpvc(ctx, LPAREN, true, 1, dvec, nelt, argok, inptok);
                if (ctx.error.lfatal) return;
                ctx.missng.mvval = dvec[0];
                break;
            case 18:  // saveprecision
                getivc(ctx, LPAREN, true, 1, ivec, nelt, argok, locok);
                if (ctx.error.lfatal) return;
                if (argok && nelt > 0) {
                    if (ivec[0] <= 0 && ivec[0] > 15) { inpter(ctx, PERROR, L.errpos.data() + 1,
                            "Value of saveprecision must be greater than zero and less than 15."); inptok = false; }
                    else ctx.savcmn.svprec = ivec[0];
                }
                break;
            case 19:  // yr2000
                gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                       "Available options for yr2000 are yes or no.", ivec, nelt, argok, inptok);
                if (ctx.error.lfatal) return;
                if (argok && nelt > 0) ctx.x11msc.yr2000 = (ivec[0] == 1);
                break;
            case 20:  // trimzero
                gtdcvc(ctx, LPAREN, true, 1, ZRODIC, zroptr, 3,
                       "Available options for trimzero are yes, span or no.", ivec, nelt, argok, inptok);
                if (ctx.error.lfatal) return;
                if (argok && nelt > 0) ltrim = ivec[0] - 1;
                break;
            case 21:  // divpower
                getivc(ctx, LPAREN, true, 1, ivec, nelt, argok, inptok);
                if (ctx.error.lfatal) return;
                if (argok && nelt > 0) {
                    if (ivec[0] < -9 || ivec[0] > 9) { inpter(ctx, PERROR, L.errpos.data() + 1,
                            "Value entered for divpower must be between -9 and 9, inclusive."); inptok = false; }
                    else ctx.x11opt.divpwr = ivec[0];
                }
                break;
            case 22:  // appendfcst
                gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                       "Available options for appending forecasts are yes or no.", ivec, nelt, argok, inptok);
                if (ctx.error.lfatal) return;
                if (argok && nelt > 0) ctx.tbllog.savfct = (ivec[0] == 1);  // getsrs.f:400
                break;
            case 23:  // appendbcst
                gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                       "Available options for appending backcasts are yes or no.", ivec, nelt, argok, inptok);
                if (ctx.error.lfatal) return;
                if (argok && nelt > 0) ctx.tbllog.savbct = (ivec[0] == 1);  // getsrs.f:409
                break;
            case 24:  // type
                gtdcvc(ctx, LPAREN, true, 1, TYPDIC, typptr, 2,
                       "Available options for type are flow or stock.", ivec, nelt, argok, inptok);
                if (ctx.error.lfatal) return;
                if (argok && nelt > 0) ctx.model.isrflw = ivec[0];
                break;
            }
            continue;  // GO TO 250 (loop)
        }
        if (ctx.error.lfatal) return;

        // --- End of argument loop: finalize series ---
        if (!havesp && hvstrt) { sp = 12; havesp = true; }
        if (!havesp && sp > 12) sp = 12;
        if (!isdate(start, sp)) {
            inpter(ctx, PERRNP, L.pos.data() + 1, "Start date not valid");
            havesp = false; locok = false;
        }
        if (spnvec[(1 - 1) * 2 + YR] != prm::NOTSET) {
            cpyint(spnvec, 2, 1, begspn);
            if (!isdate(begspn, sp)) { inpter(ctx, PERRNP, L.pos.data() + 1, "Span starting date not valid"); havesp = false; locok = false; }
        }
        if (spnvec[(2 - 1) * 2 + YR] != prm::NOTSET) {
            cpyint(spnvec + 2, 2, 1, endspn);
            if (!isdate(endspn, sp)) { inpter(ctx, PERRNP, L.pos.data() + 1, "Span ending date not valid"); havesp = false; locok = false; }
        }
        if ((spnvec[YR] == prm::NOTSET || spnvec[2 + YR] == prm::NOTSET) && ltrim == 1) {
            inpter(ctx, PERRNP, L.errpos.data() + 1, "Must specify starting and ending span when trimzero=span.");
            locok = false;
        }
        if (locok && havfil && !havsrs) {
            bool hvfreq = havesp;
            int freq = sp;
            gtfldt_free(ctx, prm::PLEN, file, nflchr, y, nobs, hvfreq, freq, hvstrt, argok, locok);
            havesp = hvfreq; sp = freq;
            if (argok) havsrs = true;
        }
        if (!havsrs) {
            if (locok) { inpter(ctx, PERRNP, L.errpos.data() + 1, "No time series specified"); locok = false; }
            else if (havfil) { inpter(ctx, PERRNP, L.errpos.data() + 1,
                    "Time series could not be read due to previously found errors"); }
        } else {
            if (spnvec[YR] == prm::NOTSET) cpyint(start, 2, 1, begspn);
            if (spnvec[2 + YR] == prm::NOTSET) addate(start, sp, nobs - 1, endspn);
            dfdate(endspn, begspn, sp, nspobs);
            nspobs = nspobs + 1;
            if (!chkcvr(start, nobs, begspn, nspobs, sp)) {
                inpter(ctx, PERRNP, L.errpos.data() + 1, "Span not within the series");
                locok = false;
            }
            if (spnmdl[YR] == prm::NOTSET) {
                cpyint(begspn, 2, 1, begmdl);
            } else {
                cpyint(spnmdl, 2, 1, begmdl);
                if (!isdate(begmdl, sp)) { inpter(ctx, PERRNP, L.pos.data() + 1, "Model span starting date not valid"); havesp = false; locok = false; }
            }
            if (spnmdl[2 + YR] == prm::NOTSET || spnmdl[2 + YR] == 0) {
                addate(begspn, sp, nspobs - 1, endmdl);
                if (spnmdl[2 + YR] == 0) {
                    endmdl[MO] = spnmdl[2 + MO];
                    if (endmdl[MO] > endspn[MO]) endmdl[YR] = endmdl[YR] - 1;
                    ctx.rev.fixper = endmdl[MO];
                }
            } else {
                cpyint(spnmdl + 2, 2, 1, endmdl);
                if (!isdate(endmdl, sp)) { inpter(ctx, PERRNP, L.pos.data() + 1, "Model span ending date not valid"); havesp = false; locok = false; }
            }
            if (hvmdsp) {
                int nmdl;
                dfdate(endmdl, begmdl, sp, nmdl);
                nmdl = nmdl + 1;
                if (!chkcvr(begspn, nspobs, begmdl, nmdl, sp)) {
                    inpter(ctx, PERRNP, L.errpos.data() + 1, "Model span not within the span of available data.");
                    inptok = false;
                }
            }
        }
        if (ctx.agr.iag == prm::NOTSET) ctx.agr.iag = -1;
        if (ctx.model.isrflw == prm::NOTSET) ctx.model.isrflw = 0;

        // --- capture for the gate ---
        if (havsrs) {
            ctx.captured.has_series = true;
            ctx.captured.period = sp;
            ctx.captured.nobs = nobs;
            ctx.captured.series_start = {start[0], start[1]};
            ctx.captured.span_start = {begspn[0], begspn[1]};
            ctx.captured.span_end = {endspn[0], endspn[1]};
            std::string t = srsttl.substr(0, static_cast<std::size_t>(nttlcr > 0 ? nttlcr : 0));
            if (!t.empty()) ctx.captured.title = t;
            std::string fn = file.substr(0, static_cast<std::size_t>(nflchr > 0 ? nflchr : 0));
            ctx.captured.data_file = fn;
        }

        inptok = inptok && locok;
        return;
    }
}

} // namespace x13
