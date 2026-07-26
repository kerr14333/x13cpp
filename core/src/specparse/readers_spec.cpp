// readers_spec.cpp -- per-spec argument readers (gtinpt dispatch targets).
//
// Each reader runs the shared gtarg loop over the spec's argument dictionary
// (verbatim from the Fortran) and consumes each argument's value token-faithfully
// so the stream stays aligned and unknown arguments are rejected exactly as the
// oracle does. Key settings (period/span already in getsrs; here transform
// function, ARIMA model orders, forecast maxlead, x11 mode, regression variable
// lists) are captured for the M1 gate. Deep option/state processing beyond
// argument parsing is deferred to later milestones.
#include "specparse/specparse.hpp"
#include "numeric/numeric.hpp"   // dpmpar (machine precision for tol checks)
#include "notset.hpp"
#include "srslen.hpp"
#include "model.hpp"   // prm::POTLR
#include "x11/loadxr.hpp"   // loadxr, xrg_clear_working (x11regression model store)
#include "composite/agr.hpp"   // agr1 (composite{} hands the aggregate over as the series)

#include <cctype>
#include <cmath>
#include <string>
#include <vector>

namespace x13 {

using namespace lexprm;

// Push one captured value token, case-folding bare NAME tokens to lowercase.
// X-13 spec input is case-insensitive (the Fortran folds keyword/enumerated-value
// tokens before the ARGDIC/gtdcvc dictionary lookups), so uppercase specs -- e.g.
// the BLS CES specs' `FUNCTION = LOG`, `SEASONALMA = (S3X5)`, `SAVE = (D10)` --
// must map to the same options as lowercase. The raw `cap[0] == "log"` comparisons
// in the readers below are case-sensitive, so fold NAME tokens here (the single
// capture seam) rather than at every comparison site. QUOTE tokens (file paths,
// titles, series names) are preserved verbatim -- those are genuinely
// case-sensitive; INTGR/DBL are numeric.
static void push_tok(const X13Context& ctx, std::vector<std::string>* cap) {
    std::string s = cur_tok(ctx);
    if (ctx.lex.nxtktp == NAME)
        for (char& c : s) c = static_cast<char>(
            std::tolower(static_cast<unsigned char>(c)));
    cap->push_back(std::move(s));
}

// Consume one argument value (the '=' was already eaten by gtarg). Mirrors the
// token consumption of skparg.f; optionally records scalar/list element tokens.
static void consume_value(X13Context& ctx, std::vector<std::string>* cap) {
    LexState& L = ctx.lex;
    if (L.nxtktp == EQUALS) lex(ctx);
    if (L.nxtktp == LPAREN || L.nxtktp == LBRAKT) {
        int clsgtp = clsgrp(L.nxtktp);
        lex(ctx);
        while (L.nxtktp != clsgtp && L.nxtktp != EOFTOK) {
            if (cap && (L.nxtktp == NAME || L.nxtktp == QUOTE ||
                        L.nxtktp == INTGR || L.nxtktp == DBL))
                push_tok(ctx, cap);
            lex(ctx);
        }
        lex(ctx);   // consume the closing bracket
    } else if (L.nxtktp == DBL || L.nxtktp == INTGR || L.nxtktp == NAME ||
               L.nxtktp == QUOTE) {
        if (cap) push_tok(ctx, cap);
        lex(ctx);
    } else {
        inpter(ctx, PERROR, L.lstpos.data() + 1,
               "Expected NAME=VALUE or NAME=(LIST) not \"" + cur_tok(ctx) + "\"");
        abend(ctx);
    }
}

// Generic reader: consume every argument's value (token-faithful) and reject
// unknown arguments via gtarg.
void gt_generic(X13Context& ctx, std::string_view argdic, const int* argptr,
                int narg, bool& inptok) {
    std::vector<int> arglog(static_cast<std::size_t>(2 * narg));
    for (auto& v : arglog) v = -32767;  // NOTSET
    int argidx;
    while (gtarg(ctx, argdic, argptr, narg, argidx, arglog.data(), inptok)) {
        if (ctx.error.lfatal) return;
        consume_value(ctx, nullptr);
        if (ctx.error.lfatal) return;
    }
}

// ---- transform{} (getadj.f) : capture function ----------------------------
void gt_transform(X13Context& ctx, bool& inptok) {
    constexpr int PARG = 20;
    static const char ARGDIC[] =
        "datastarttitlefileformatadjustadjustregpowerfunctionprintsavemode"
        "nametypeprecisionsavelogaicdifftrimzerotemppriortrendconstant";
    static const int argptr[PARG + 1] = {1, 5, 10, 15, 19, 25, 31, 40, 45, 53, 58,
        62, 66, 70, 74, 83, 90, 97, 105, 119, 127};
    int arglog[2 * PARG];
    for (auto& v : arglog) v = -32767;
    int argidx;
    // getadj.f user prior-adjustment capture (minimal slice: inline data=, ratio
    // mode, type=permanent). data(1)/start(2)/mode(12)/type(14).
    std::vector<double> pdata;
    int pr_type = 0;     // 0 unset; 1 temporary, 2 permanent (TYPDIC order)
    int pr_mode = 0;     // 0 percent, 1 ratio, 2 diff (Percnt after -1); default percent
    int pr_start[2] = {prm::NOTSET, prm::NOTSET};
    std::string pr_file;                 // file= : the factors live in a file
    bool pr_fmt = false;                 // format= : formatted read, unported
    bool pr_multi = false;               // more than one prior set (Nprtyp>1)
    while (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
        if (ctx.error.lfatal) return;
        // Capture the value tokens for the args whose state we set here:
        //   6 = adjust, 8 = power, 9 = function, 11 = save, 17 = aicdiff,
        //   1 = data, 2 = start, 4 = file, 5 = format, 12 = mode, 14 = type
        //   (user prior adjustment).
        std::vector<std::string> cap;
        bool want = (argidx == 6 || argidx == 8 || argidx == 9 || argidx == 11 ||
                     argidx == 17 || argidx == 1 || argidx == 2 || argidx == 4 ||
                     argidx == 5 || argidx == 12 || argidx == 14 || argidx == 19 ||
                     argidx == 20);
        consume_value(ctx, want ? &cap : nullptr);
        if (ctx.error.lfatal) return;
        if (argidx == 1) {                       // data= : inline prior factors
            for (const auto& t : cap) {
                try { pdata.push_back(std::stod(t)); } catch (...) {}
            }
        } else if (argidx == 2) {                // start= : YYYY.MM
            if (!cap.empty()) {
                try {
                    double d = std::stod(cap[0]);
                    pr_start[0] = static_cast<int>(d);
                    pr_start[1] = static_cast<int>((d - pr_start[0]) * 100.0 + 0.5);
                } catch (...) {}
            }
        } else if (argidx == 4 && !cap.empty()) { // file= : prior factors on disk
            pr_file = cap[0];
            if (cap.size() > 1) pr_multi = true;
        } else if (argidx == 5 && !cap.empty()) { // format= : formatted read
            pr_fmt = true;
        } else if (argidx == 12 && !cap.empty()) { // mode= percent|ratio|diff
            if (cap[0] == "percent") pr_mode = 0;
            else if (cap[0] == "ratio") pr_mode = 1;
            else if (cap[0] == "diff") pr_mode = 2;
            if (cap.size() > 1) pr_multi = true;
        } else if (argidx == 19 && !cap.empty()) { // temppriortrend= yes|no
            // getadj.f:421 -- put the temporary prior back into the final TREND
            // (D12) as well as the SA series.
            ctx.prior.lprntr = (cap[0] == "yes");
        } else if (argidx == 20 && !cap.empty()) { // constant=
            // getadj.f:177-436 -- a positive constant added to the WHOLE series
            // (editor.f:430) so a multiplicative adjustment can run on data that
            // would otherwise touch or cross zero. It is taken back out of the
            // published D11/D12/original at the x11pt3 output points.
            double c = 0.0;
            bool ok = true;
            try { c = std::stod(cap[0]); } catch (...) { ok = false; }
            if (!ok || c <= 0.0) {
                inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                       "Constant argument cannot be less than or equal to zero.");
                inptok = false;
            } else {
                ctx.adj.cnstnt = c;
            }
        } else if (argidx == 14 && !cap.empty()) { // type= temporary|permanent
            const std::string& t = cap[0];
            if (t == "temporary" || t == "temp") pr_type = 1;
            else if (t == "permanent" || t == "perm") pr_type = 2;
            if (cap.size() > 1) pr_multi = true;
        }
        if (argidx == 6 && !cap.empty()) {
            // getadj.f adjust= mapping (ADJDIC='nonelomloqlpyear'): 1 none, 2 lom,
            // 3 loq, 4 lpyear; then normalize lom/loq to the seasonal period.
            const std::string& a = cap[0];
            int priadj = 0;
            if (a == "none")        priadj = 1;
            else if (a == "lom")    priadj = 2;
            else if (a == "loq")    priadj = 3;
            else if (a == "lpyear") priadj = 4;
            if (priadj > 0) {
                if (priadj == 2 && ctx.model.sp == 4) priadj = 3;
                if (priadj == 3 && ctx.model.sp == 12) priadj = 2;
                ctx.prior.priadj = priadj;
            }
        }
        if (argidx == 9 && !cap.empty()) {
            // getadj.f function= mapping (FCNDIC='logsqrtlogisticnoneinverseauto').
            const std::string& f = cap[0];
            ctx.captured.transform_function = f;
            if (f == "log")           { ctx.arima.fcntyp = 1; ctx.arima.lam = 0.0; }
            else if (f == "sqrt")     { ctx.arima.fcntyp = 6; ctx.arima.lam = 0.5; }
            else if (f == "none")     { ctx.arima.fcntyp = 4; ctx.arima.lam = 1.0; }
            else if (f == "inverse")  { ctx.arima.fcntyp = 6; ctx.arima.lam = -1.0; }
            else if (f == "logistic") { ctx.arima.fcntyp = 3; ctx.arima.lam = prm::DNOTST; }
            else if (f == "auto")     { ctx.arima.fcntyp = 0; ctx.arima.lam = prm::DNOTST; }
        } else if (argidx == 8 && !cap.empty()) {
            // getadj.f power= : Box-Cox parameter, Fcntyp=5, Lam=value.
            try {
                ctx.captured.transform_power = std::stod(cap[0]);
                ctx.arima.lam = ctx.captured.transform_power;
                ctx.arima.fcntyp = 5;
            } catch (...) { /* malformed handled by the Fortran error path */ }
        } else if (argidx == 11) {
            for (const auto& t : cap) ctx.captured.save_tables.push_back(t);
        } else if (argidx == 17 && !cap.empty()) {
            // getadj.f:398 aicdiff= : the transform AICC threshold (Traicd).
            try {
                ctx.arima.traicd = std::stod(cap[0]);
            } catch (...) { /* malformed handled by the Fortran error path */ }
        }
    }
    // getadj.f:479-530 -- the factors can come from a file instead of data=. One
    // file, one type, free format (the multi-file / multi-type form and the
    // formatted read are not ported; both fatal rather than silently reading the
    // wrong thing).
    if (pr_multi) {
        // getadj.f handles up to PNADJ prior sets (one file/type/mode each); only
        // one is ported. Fatal rather than silently using the first: the dropped
        // set would change every table without any sign that it was ignored.
        inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
               "transform: more than one set of prior adjustment factors "
               "(Nprtyp>1) not yet supported.");
        inptok = false;
    } else if (!pr_file.empty()) {
        if (!pdata.empty()) {
            inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                   "transform data= and file= cannot both be given.");
            inptok = false;
        } else if (pr_fmt) {
            inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                   "transform format= (formatted prior-factor read) not yet "
                   "supported.");
            inptok = false;
        } else {
            std::vector<double> buf(prm::PLEN, 0.0);
            int nobs = 0;
            bool hvfreq = false, argok = true;
            int freq = ctx.model.sp;
            gtfldt_free(ctx, prm::PLEN, pr_file,
                        static_cast<int>(pr_file.size()), buf.data(), nobs,
                        hvfreq, freq, pr_start[0] != prm::NOTSET, argok, inptok);
            if (argok && nobs > 0) pdata.assign(buf.begin(), buf.begin() + nobs);
        }
    }
    // getadj.f:442-580 -- store the user prior-adjustment factors, permanent
    // (Usrpad) or temporary (Usrtad). One set, percent or ratio mode; the diff
    // mode (Percnt==2, additive factors) is not ported -> fatal cleanly.
    if (!pdata.empty()) {
        if (pr_type == 0) pr_type = 2;   // getadj.f:452 default: permanent
        if (pr_mode == 2) {
            inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                   "transform mode=diff prior factors not yet supported.");
            inptok = false;
        } else {
            const int n = static_cast<int>(pdata.size());
            const bool temp = (pr_type == 1);
            // addadj.f:47-56 converts percent -> ratio; done here instead, which
            // is the same single conversion (the oracle rewrites Usradj in place).
            double* up = temp ? ctx.priadj.usrtad.data() : ctx.priadj.usrpad.data();
            for (int i = 0; i < n; ++i)
                up[i] = (pr_mode == 0) ? pdata[i] / 100.0 : pdata[i];
            int* bgu = temp ? ctx.priusr.bgutad.data() : ctx.priusr.bgupad.data();
            if (temp) { ctx.priusr.nustad = n; ctx.priusr.ntser = 7; }  // "TempAdj"
            else      { ctx.priusr.nuspad = n; ctx.priusr.npser = 7; }  // "PermAdj"
            if (pr_start[0] != prm::NOTSET) {
                bgu[0] = pr_start[0];
                bgu[1] = pr_start[1];
            } else {                 // getadj.f:143 default = series start (Begsrs)
                bgu[0] = ctx.arima.begsrs(1);
                bgu[1] = ctx.arima.begsrs(2);
            }
        }
    }
}

// ---- arima{} (gtarma.f; model= parsed for real via getmdl.f) --------------
void gt_arima(X13Context& ctx, bool& inptok) {
    LexState& L = ctx.lex;
    constexpr int PARG = 5;
    static const char ARGDIC[] = "titlemodeldiffarma";  // title model diff ar ma
    static const int argptr[PARG + 1] = {1, 6, 11, 15, 17, 19};
    int arglog[2 * PARG];
    for (auto& v : arglog) v = -32767;
    bool havmdl = false;
    int argidx;
    while (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
        if (ctx.error.lfatal) return;
        if (argidx == 2) {          // model -> getmdl.f (builds Mdl/Opr/Arima*)
            bool argok = true;
            getmdl(ctx, argok, inptok, false);
            havmdl = true;
        } else if (argidx >= 3) {   // diff / ar / ma -> gtinvl.f (gtarma.f:67)
            // The displacement is 2: the two arguments (title, model) ahead of
            // diff in ARGDIC, so argidx-2 is prm::DIFF/AR/MA. gtarg has already
            // consumed the "="; Lprtdf (argidx==3) is print surface.
            gtinvl(ctx, argidx - 2, inptok);
        } else {                     // title
            consume_value(ctx, nullptr);
        }
        if (ctx.error.lfatal) return;
    }
    if (ctx.error.lfatal) return;
    if (!havmdl) {
        inpter(ctx, PERROR, L.errpos.data() + 1,
               "An ARIMA spec was found with no specified model. If an ARIMA model "
               "of (0 0 0) was intended,please specify it using the model argument.");
        inptok = false;
    }
    // gtarma.f: check if the regression and arima models are fixed.
    mdlfix(ctx);
}

// ---- forecast{} (gtfcst.f) : capture maxlead ------------------------------
void gt_forecast(X13Context& ctx, bool& inptok) {
    constexpr int PARG = 7;
    static const char ARGDIC[] = "excludemaxleadprobabilityprintsavemaxbacklognormal";
    static const int argptr[PARG + 1] = {1, 8, 15, 26, 31, 35, 42, 51};
    int arglog[2 * PARG];
    for (auto& v : arglog) v = -32767;
    int argidx;
    while (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
        if (ctx.error.lfatal) return;
        std::vector<std::string> cap;
        // gtfcst.f: 1 exclude->Fctdrp, 2 maxlead->Nfcst, 3 probability->Ciprob,
        // 6 maxback->Nbcst, 7 lognormal->Lognrm.
        bool want = (argidx == 1 || argidx == 2 || argidx == 3 || argidx == 6 ||
                     argidx == 7);
        consume_value(ctx, want ? &cap : nullptr);
        if (ctx.error.lfatal) return;
        if (!cap.empty()) {
            if (argidx == 3) {                       // probability (real)
                try { ctx.arima.ciprob = std::stod(cap[0]); } catch (...) {}
            } else if (argidx == 7) {                // lognormal (yes/no)
                ctx.arima.lognrm = (cap[0] == "yes" || cap[0] == "1");
            } else {                                 // integer-valued args
                int v = 0;
                bool ok = true;
                try { v = std::stoi(cap[0]); } catch (...) { ok = false; }
                if (ok) {
                    if (argidx == 1) {
                        ctx.arima.fctdrp = v;
                    } else if (argidx == 2) {
                        ctx.captured.forecast_maxlead = v;
                        if (v <= prm::PFCST) ctx.extend.nfcst = v;
                    } else if (argidx == 6) {
                        if (v <= prm::PFCST) ctx.extend.nbcst = v;
                    }
                }
            }
        }
    }
}

// ---- x11{} (getx11.f) : capture mode --------------------------------------
void gt_x11(X13Context& ctx, bool havesp, bool& inptok) {
    constexpr int PARG = 25;
    static const char ARGDIC[] =
        "modesigmalimseasonalmatrendmatitleextremeadjtypeappendfcsttrendic"
        "calendarsigmasigmavecx11eastertaperkeepholidayfinalsfshortprintsave"
        "savelogprint1stpassexcludefcsttrue7termshrinkcenterseasonalappendbcst";
    static const int argptr[PARG + 1] = {1, 5, 13, 23, 30, 35, 45, 49, 59, 66, 79,
        87, 96, 101, 112, 117, 124, 129, 133, 140, 152, 163, 172, 178, 192, 202};
    // seasonalma filter dictionary (getx11.f:76): index i -> filter code i-1
    // (x11default=0 s3x3=1 s3x5=2 s3x9=3 s3x15=4 stable=5 msr=6 s3x1=7).
    static const char SFDIC[] = "x11defaults3x3s3x5s3x9s3x15stablemsrs3x1";
    static const int sfptr[9] = {1, 11, 15, 19, 23, 28, 34, 37, 41};
    int arglog[2 * PARG];
    for (auto& v : arglog) v = -32767;
    int argidx;
    while (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
        if (ctx.error.lfatal) return;
        if (argidx == 2) {
            // sigmalim -> Sigml/Sigmu (getx11.f:200-224). The extreme-value
            // weighting (xtrm/wtxtrm) already honours Sigml/Sigmu.
            double sigl[2] = {prm::DNOTST, prm::DNOTST};
            int nelt = 0;
            bool argok = true;
            gtdpvc(ctx, LPAREN, false, 2, sigl, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (nelt == 1) {
                inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                       "Two sigma limits needed (or use a comma as place "
                       "holder).");
                inptok = false;
            } else if (nelt > 0) {
                if (dpeq(sigl[0], prm::DNOTST)) sigl[0] = 1.5;
                if (dpeq(sigl[1], prm::DNOTST)) sigl[1] = 2.5;
                if (sigl[0] <= 0.0 || sigl[1] <= 0.0) {
                    inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                           "Sigma limits must be greater than zero.");
                    inptok = false;
                } else if (sigl[0] > sigl[1]) {
                    inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                           "Lower sigma limit must be less than upper sigma "
                           "limit.");
                    inptok = false;
                } else {
                    ctx.x11opt.sigml = sigl[0];
                    ctx.x11opt.sigmu = sigl[1];
                }
            }
            continue;
        }
        if (argidx == 4) {
            // trendma -> Ktcopt (fixed Henderson length; getx11.f:283-295). vtc
            // honours Ktcopt>0 as the trend-filter length.
            int ivec[1] = {prm::NOTSET};
            int nelt = 0;
            bool argok = true;
            getivc(ctx, LPAREN, true, 1, ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) {
                if (ivec[0] % 2 == 0 || ivec[0] <= 0) {
                    inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                           "Length of Henderson trend filter must be a positive "
                           "odd integer.");
                    inptok = false;
                } else {
                    ctx.x11opt.ktcopt = ivec[0];
                }
            }
            continue;
        }
        if (argidx == 3) {
            // seasonalma -> Lterm/Lter (getx11.f:228-268). The seasonal filter
            // (vsfa) already honours Lterm/Lter; only this parse wiring was
            // missing.
            int isf[prm::PSP];
            setint(prm::NOTSET, prm::PSP, isf);
            int nelt = 0;
            bool argok = true;
            gtdcvc(ctx, LPAREN, false, prm::PSP, SFDIC, sfptr, 8,
                   "Improper value(s) entered for seasonalma.", isf, nelt,
                   argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) {
                const int sp = ctx.model.sp;
                if (!havesp) {
                    inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                           "No seasonal period specified in series spec.");
                    inptok = false;
                } else if (nelt == 1) {
                    ctx.x11opt.lterm = isf[0] - 1;
                    for (int i = 1; i <= sp; ++i)
                        ctx.x11opt.lter(i) = ctx.x11opt.lterm;
                } else if (nelt == sp) {
                    if (isf[0] == prm::NOTSET) isf[0] = 6;
                    ctx.x11opt.lterm = isf[0] - 1;
                    ctx.x11opt.lter(1) = ctx.x11opt.lterm;
                    for (int i = 2; i <= sp; ++i)
                        ctx.x11opt.lter(i) =
                            (isf[i - 1] == prm::NOTSET) ? ctx.x11opt.lterm
                                                        : isf[i - 1] - 1;
                } else {
                    inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                           "Specify either 1 or Sp values for seasonalma.");
                    inptok = false;
                }
            }
            continue;
        }
        if (argidx == 6) {
            // extremeadj -> Imad (getx11.f:325-333). sdxtrm honours Imad
            // (std=0 wmad=1 wmadlog=2 tau=3 taulog=4).
            static const char OTLDIC[] = "stdwmadwmadlogtautaulog";
            static const int otlptr[6] = {1, 4, 8, 15, 18, 24};
            int ivec[1] = {prm::NOTSET};
            int nelt = 0;
            bool argok = true;
            gtdcvc(ctx, LPAREN, true, 1, OTLDIC, otlptr, 5,
                   "Improper X-11 outlier option: valid choices for extremeadj "
                   "are", ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) ctx.x11opt.imad = ivec[0] - 1;
            continue;
        }
        if (argidx == 9) {
            // trendic -> Tic (I/C ratio; getx11.f:366-375). vtc honours Tic.
            double dvec[1] = {prm::DNOTST};
            int nelt = 0;
            bool argok = true;
            gtdpvc(ctx, LPAREN, true, 1, dvec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) {
                if (dvec[0] <= 0.0) {
                    inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                           "Specified I/C ratio must be greater than zero.");
                    inptok = false;
                } else {
                    ctx.x11opt.tic = dvec[0];
                }
            }
            continue;
        }
        if (argidx == 12) {
            // x11easter -> Keastr (getx11.f:470-474). YSNDIC yes=1/no=2 ->
            // Keastr=2-ivec (yes=1). The editor step (below, in run_x11) maps
            // Keastr>=1 to Khol=1 + Lgenx=T so the classic X-11 Easter estimation
            // runs; a monthly-only feature.
            static const char YSNDIC[] = "yesno";
            static const int ysnptr[3] = {1, 4, 6};
            int ivec[1] = {prm::NOTSET};
            int nelt = 0;
            bool argok = true;
            gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                   "Available options for x11easter are yes or no.", ivec, nelt,
                   argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) ctx.x11opt.keastr = 2 - ivec[0];
            continue;
        }
        if (argidx == 11) {
            // sigmavec -> Csigvc (getx11.f:390-427). Per-period calendarsigma=
            // select flags. SUMDIC maps a month/quarter name to a 1..28 index;
            // monthly full names (13-24) fold to 1..12 and quarter names (25-28)
            // to 1..4. xtrm honours Csigvc(period) when Ksdev==4.
            static const char SUMDIC[] =
                "janfebmaraprmayjunjulaugsepoctnovdec"
                "januaryfebruarymarchaprilmayjunejulyaugustseptember"
                "octobernovemberdecemberq1q2q3q4";
            static const int sumptr[29] = {1, 4, 7, 10, 13, 16, 19, 22, 25, 28,
                31, 34, 37, 44, 52, 57, 62, 65, 69, 73, 79, 88, 95, 103, 111,
                113, 115, 117, 119};
            int calidx[prm::PSP];
            setint(prm::NOTSET, prm::PSP, calidx);
            int nelt = 0;
            bool argok = true;
            gtdcvc(ctx, LPAREN, true, prm::PSP, SUMDIC, sumptr, 28,
                   "Improper value(s) entered for sigmavec.", calidx, nelt,
                   argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) {
                const int sp = ctx.model.sp;
                if (!havesp) {
                    inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                           "No seasonal period specified in series spec.");
                    inptok = false;
                } else {
                    for (int i = 0; i < nelt; ++i) {
                        int isvc = calidx[i];
                        if (isvc >= 13 && isvc <= 24 && sp == 12) {
                            isvc -= 12;
                        } else if (isvc >= 25 && sp == 4) {
                            isvc -= 24;
                        } else {
                            if (sp == 12 && isvc >= 25) {
                                inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                                       "Entry for sigmavec not valid for monthly "
                                       "data.");
                                inptok = false;
                                isvc = prm::NOTSET;
                            } else if (sp == 4 && isvc < 25) {
                                inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                                       "Entry for sigmavec not valid for "
                                       "quarterly data.");
                                inptok = false;
                                isvc = prm::NOTSET;
                            }
                        }
                        if (isvc > 0) ctx.xtrm.csigvc(isvc) = true;
                    }
                }
            }
            continue;
        }
        if (argidx == 23) {
            // shrink -> Ishrnk (getx11.f:459-465). SHKDIC='nonegloballocal',
            // Ishrnk=ivec(1)-1 (none=0 global=1 local=2). x11pt3 calls shrink()
            // on the final seasonal factors when Ishrnk>0.
            static const char SHKDIC[] = "nonegloballocal";
            static const int shkptr[4] = {1, 5, 11, 16};
            int ivec[1] = {prm::NOTSET};
            int nelt = 0;
            bool argok = true;
            gtdcvc(ctx, LPAREN, true, 1, SHKDIC, shkptr, 3,
                   "Entry for shrink argument must be none, global or local.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) ctx.x11opt.ishrnk = ivec[0] - 1;
            continue;
        }
        if (argidx == 7) {
            // type -> Kfulsm (getx11.f:339-343). TYPDIC='sasummarytrend',
            // Kfulsm=ivec(1)-1 (sa=0 summary=1 trend=2). The X-11 spine already
            // branches on Kfulsm throughout (x11parts/x11drv/x11filt).
            static const char TYPDIC[] = "sasummarytrend";
            static const int typptr[4] = {1, 3, 10, 15};
            int ivec[1] = {prm::NOTSET};
            int nelt = 0;
            bool argok = true;
            gtdcvc(ctx, LPAREN, true, 1, TYPDIC, typptr, 3,
                   "The available adjustment types are sa, summary, or trend.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) ctx.x11opt.kfulsm = ivec[0] - 1;
            continue;
        }
        if (argidx == 10) {
            // calendarsigma -> Ksdev (getx11.f:381-385). xtrm honours Ksdev
            // (none=1 signif=2 all=3 select=4).
            static const char BNDDIC[] = "nonesignifallselect";
            static const int bndptr[5] = {1, 5, 11, 14, 20};
            int ivec[1] = {prm::NOTSET};
            int nelt = 0;
            bool argok = true;
            gtdcvc(ctx, LPAREN, true, 1, BNDDIC, bndptr, 4,
                   "Available options for calendarsigma are none, signif, all "
                   "or select.", ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) ctx.xtrm.ksdev = ivec[0];
            continue;
        }
        if (argidx == 8 || argidx == 25) {
            // appendfcst -> Savfct (getx11.f:348-352); appendbcst -> Savbct
            // (getx11.f:452-456). Same globals series{}/seats{} set: they widen
            // the save/punch range of the X-11 tables to the forecast (backcast)
            // span, they do not change any arithmetic.
            static const char YSNDIC[] = "yesno";
            static const int ysnptr[3] = {1, 4, 6};
            int ivec[1] = {prm::NOTSET};
            int nelt = 0;
            bool argok = true;
            gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                   argidx == 8
                       ? "Available options for appending forecasts are yes or no."
                       : "Available options for appending backcasts are yes or no.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) {
                if (argidx == 8)
                    ctx.tbllog.savfct = (ivec[0] == 1);
                else
                    ctx.tbllog.savbct = (ivec[0] == 1);
            }
            continue;
        }
        if (argidx == 14) {
            // keepholiday -> Finhol (getx11.f:258-264). NOTE the inverted sense:
            // Finhol=ivec(1).eq.2, i.e. keepholiday=NO leaves the holiday effect
            // in the FINAL adjustment (Fin* = "remove in the final SA").
            static const char YSNDIC[] = "yesno";
            static const int ysnptr[3] = {1, 4, 6};
            int ivec[1] = {prm::NOTSET};
            int nelt = 0;
            bool argok = true;
            gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                   "Available options for keepholiday are yes or no.", ivec, nelt,
                   argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) ctx.x11adj.finhol = (ivec[0] == 2);
            continue;
        }
        if (argidx == 16 || argidx == 21 || argidx == 22 || argidx == 24) {
            // The four NUMERIC yes/no x11{} switches that share getx11.f's YSNDIC
            // shape (`flag = ivec(1).eq.1`). Every one of them was already ported
            // in the X-11 engine but had no parse seam, so the argument was
            // accepted and silently dropped -- the run came back OUTCOME: OK
            // having done the default thing (excludefcst cost ~3.5e-3 in d13).
            //   16 sfshort        -> Shrtsf (getx11.f:527-533)  short seasonal MA
            //   21 excludefcst    -> Noxfct (getx11.f:551-557)  si/xtrm extreme-
            //        value span: keep the forecast/backcast rows OUT of the
            //        extreme-value replacement window (x11pt2.f:469/636/747)
            //   22 true7term      -> Tru7hn (getx11.f:560-566)  hndtrn 7-term
            //   24 centerseasonal -> Lcentr (getx11.f:441-447)
            // (getx11.f:355-361's print1stpass -> Prt1ps is print surface only;
            // it is not in /x11msc/ and falls through to consume_value below.)
            static const char YSNDIC[] = "yesno";
            static const int ysnptr[3] = {1, 4, 6};
            const char* msg =
                argidx == 16 ? "Available options for sfshort are yes or no."
                : argidx == 21 ? "Available options for excludefcst are yes or no."
                : argidx == 22 ? "Available options for true7term are yes or no."
                               : "Available options for centerseasonal are yes or no.";
            int ivec[1] = {prm::NOTSET};
            int nelt = 0;
            bool argok = true;
            gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2, msg, ivec, nelt,
                   argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) {
                const bool on = (ivec[0] == 1);
                switch (argidx) {
                case 16: ctx.x11msc.shrtsf = on; break;
                case 21: ctx.x11msc.noxfct = on; break;
                case 22: ctx.x11msc.tru7hn = on; break;
                default: ctx.x11msc.lcentr = on; break;
                }
            }
            continue;
        }
        if (argidx == 15) {
            // final -> Finao/Finls/Finusr/Fintc (getx11.f:287-311). FINDIC is a
            // LIST argument (up to PFIN=4 entries): each named effect is removed
            // from the final seasonally adjusted series instead of being left in.
            constexpr int PFIN = 4;
            static const char FINDIC[] = "aolsusertc";
            static const int finptr[PFIN + 1] = {1, 3, 5, 9, 11};
            int finind[PFIN];
            setint(prm::NOTSET, PFIN, finind);
            int nelt = 0;
            bool argok = true;
            gtdcvc(ctx, LPAREN, true, PFIN, FINDIC, finptr, PFIN,
                   "Choices for final argument are ao, ls, tc, or user.", finind,
                   nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) {
                for (int i = 0; i < nelt; ++i) {
                    switch (finind[i]) {
                    case 1: ctx.x11adj.finao = true; break;
                    case 2: ctx.x11adj.finls = true; break;
                    case 3: ctx.x11adj.finusr = true; break;
                    case 4: ctx.x11adj.fintc = true; break;
                    default: break;
                    }
                }
            }
            continue;
        }
        std::vector<std::string> cap;
        consume_value(ctx, argidx == 1 ? &cap : nullptr);   // 1 = mode
        if (ctx.error.lfatal) return;
        if (argidx == 1 && !cap.empty()) ctx.captured.x11_mode = cap[0];
    }
    ctx.captured.has_x11 = true;
}

// ---- regression{} (getreg.f) : variables parsed for real via gtpdrg.f -----
void gt_regression(X13Context& ctx, bool havsrs, bool havesp, bool& havtd,
                   bool& inptok) {
    LexState& L = ctx.lex;
    constexpr int PARG = 23;
    static const char ARGDIC[] =
        "variablesuserdatastartfileformatbprintsaveaictesteastermeansnoapply"
        "usertypetcrateaicdiffsavelogcenteruserchi2testchi2testcvtlimit"
        "pvaictesttestalleastertrendtc";
    static const int argptr[PARG + 1] = {1, 10, 14, 18, 23, 27, 33, 34, 39, 43, 50,
        61, 68, 76, 82, 89, 96, 106, 114, 124, 130, 139, 152, 159};
    int arglog[2 * PARG];
    for (auto& v : arglog) v = -32767;
    bool havhol = false, havln = false, havlp = false;
    // User-defined regressor parse state (getreg.f). usrttl/usrptr feed
    // ctx.usrreg; the X matrix lands in ctx.arima.userx; ctx.arima.bgusrx holds
    // its start date. Only the default (untyped -> PRGTUD) path is built here.
    bool hvuttl = false, haveux = false, hvstrt = false;
    bool hvfile = false, havfmt = false;
    int neltux = 0, nusrrg = 0, nflchr = 0, nfmtch = 0;
    std::string rgfile(static_cast<std::size_t>(stdio::PFILCR), ' ');
    std::string rgfmt(static_cast<std::size_t>(stdio::PFILCR), ' ');
    std::string usrttl(static_cast<std::size_t>(prm::PUREG * prm::PCOLCR), ' ');
    // usertype dictionary (getreg.f:90). 16 entries.
    static const char URGDIC[] =
        "constantseasonaltdlomloqlpyearholidayholiday2holiday3holiday4"
        "holiday5aolssotransitoryuser";
    static const int urgptr[17] = {1, 9, 17, 19, 22, 25, 31, 38, 46, 54, 62,
        70, 72, 74, 76, 86, 90};
    constexpr int PURG = 16;
    // centeruser dictionary (getreg.f:103).
    static const char URRDIC[] = "meanseasonal";
    static const int urrptr[3] = {1, 5, 13};
    bool lumean = false, luseas = false;
    int iuhl[prm::PUHLGP] = {0};   // user holiday-group usage (getreg.f:137)
    // b= scratch (getreg.f:129/133: nbvec starts at NOTSET, fixvec all FALSE).
    // bvec is deliberately NOT initialized -- gtrgvl's NULL elements skip a slot
    // without writing, and getreg.f:551's writeback then copies whatever is
    // there. The Fortran reads its uninitialized stack; a NULL in the list is
    // therefore only meaningful with a fully-specified list, which is the
    // documented usage. Zero-init here so the C++ is at least deterministic.
    int nbvec = prm::NOTSET;
    bool fixvec[prm::PB] = {false};
    double bvec[prm::PB] = {0.0};
    int argidx;
    while (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
        if (ctx.error.lfatal) return;
        if (argidx == 1) {           // variables -> gtpdrg (build the groups)
            // getreg.f: the '=' was consumed by gtarg; gtpdrg reads the list.
            if (L.nxtktp == lexprm::EQUALS) lex(ctx);
            bool locok = true;
            gtpdrg(ctx, ctx.arima.begsrs.data(), ctx.arima.endmdl.data(),
                   ctx.arima.nobs, havsrs, havesp, false, havtd, havhol, havln,
                   havlp, locok, inptok);
            if (ctx.error.lfatal) return;
        } else if (argidx == 2) {    // user -- names/# columns (getreg.f:165)
            if (L.nxtktp == lexprm::EQUALS) lex(ctx);
            bool argok = true;
            gtnmvc(ctx, LPAREN, true, prm::PUREG, usrttl,
                   ctx.usrreg.usrptr.data(), ctx.usrreg.ncusrx, prm::PCOLCR,
                   argok, inptok);
            if (ctx.error.lfatal) return;
            hvuttl = argok && ctx.usrreg.ncusrx > 0;
        } else if (argidx == 3) {    // data -- the X matrix (getreg.f:175)
            if (hvfile)
                inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                       "Getting data from a file");
            if (L.nxtktp == lexprm::EQUALS) lex(ctx);
            bool argok = true;
            gtdpvc(ctx, LPAREN, true, prm::PUSERX, ctx.arima.userx.data(),
                   neltux, argok, inptok);
            if (ctx.error.lfatal) return;
            haveux = argok && neltux > 0;
        } else if (argidx == 5) {    // file -- read X matrix from a file (getreg.f:189)
            if (haveux)
                inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                       "Already have user regression");
            if (L.nxtktp == lexprm::EQUALS) lex(ctx);
            bool argok = true; int nelt = 0; int tmpptr[2];
            gtnmvc(ctx, LPAREN, true, 1, rgfile, tmpptr, nelt, stdio::PFILCR,
                   argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) {
                eltlen(ctx, 1, tmpptr, nelt, nflchr);
                if (ctx.error.lfatal) return;
                hvfile = true;
            }
        } else if (argidx == 6) {    // format (getreg.f:203)
            if (L.nxtktp == lexprm::EQUALS) lex(ctx);
            bool argok = true; int nelt = 0; int tmpptr[2];
            gtnmvc(ctx, LPAREN, true, 1, rgfmt, tmpptr, nelt, stdio::PFILCR,
                   argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok) {
                eltlen(ctx, 1, tmpptr, nelt, nfmtch);
                if (ctx.error.lfatal) return;
                havfmt = true;
            }
        } else if (argidx == 7) {    // b -> gtrgvl.f (getreg.f:215)
            if (L.nxtktp == lexprm::EQUALS) lex(ctx);
            gtrgvl(ctx, nbvec, fixvec, bvec, inptok);
            if (ctx.error.lfatal) return;
        } else if (argidx == 4) {    // start -- X matrix begin date (getreg.f:182)
            if (L.nxtktp == lexprm::EQUALS) lex(ctx);
            bool argok = true; int nelt = 0;
            gtdtvc(ctx, havesp, ctx.model.sp, LPAREN, false, 1,
                   ctx.arima.bgusrx.data(), nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            hvstrt = argok && nelt > 0;
        } else if (argidx == 13) {   // usertype (getreg.f:321)
            if (L.nxtktp == lexprm::EQUALS) lex(ctx);
            bool argok = true;
            int urgidx[prm::PUREG];
            gtdcvc(ctx, LPAREN, false, prm::PUREG, URGDIC, urgptr, PURG,
                   "Improper entry for usertype.", urgidx, nusrrg, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nusrrg > 0) {
                for (int i = 1; i <= nusrrg; ++i) {
                    int u = urgidx[i - 1];
                    int& ut = ctx.usrreg.usrtyp(i);
                    if (u == 1) ut = prm::PRGUCN;
                    else if (u == 2) ut = prm::PRGTUS;
                    else if (u == 3) { ut = prm::PRGUTD; havtd = true; }
                    else if (u == 4) { ut = prm::PRGULM; havln = true; }
                    else if (u == 5) { ut = prm::PRGULQ; havln = true; }
                    else if (u == 6) { ut = prm::PRGULY; havlp = true; }
                    else if (u >= 7 && u <= 11) {
                        ut = (u == 7) ? prm::PRGTUH : prm::PRGUH2 + (u - 8);
                        havhol = true;
                        if (iuhl[u - 7] == 0) iuhl[u - 7] = 1;  // iuhl(u-6)
                    } else if (u == 12) ut = prm::PRGUAO;
                    else if (u == 13) ut = prm::PRGULS;
                    else if (u == 14) ut = prm::PRGUSO;
                    else if (u == 15) ut = prm::PRGUCY;
                    else ut = prm::PRGTUD;   // 16/user or unset
                }
            }
        } else if (argidx == 17) {   // centeruser (getreg.f:376)
            if (L.nxtktp == lexprm::EQUALS) lex(ctx);
            bool argok = true; int ivec[1]; int nelt = 0;
            gtdcvc(ctx, LPAREN, false, 1, URRDIC, urrptr, 2,
                   "Choices for centeruser are mean and seasonal.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) {
                lumean = ivec[0] == 1;
                luseas = ivec[0] == 2;
            }
        } else {
            std::vector<std::string>* cap = nullptr;
            std::vector<std::string> tmp;
            if (argidx == 9 || argidx == 10) cap = &tmp;   // 9=save, 10=aictest
            consume_value(ctx, cap);
            if (ctx.error.lfatal) return;
            if (argidx == 9)
                for (const auto& t : tmp) ctx.captured.save_tables.push_back(t);
            if (argidx == 10) {
                ctx.captured.aictest_vars = tmp;
                // getreg.f:240-277: map each aictest token to its test flag so the
                // explicit-model aictest path (arima.f:569) can run. The automd
                // path reads aictest_vars directly and is unaffected. Each family
                // (td / easter / lom) may be specified at most once (getreg.f:
                // 244-272).
                const int* aep = ctx.lex.errpos.data() + 1;
                auto& A = ctx.arima;
                for (const auto& t0 : tmp) {
                    std::string s;
                    for (char c : t0)
                        s += static_cast<char>(
                            std::tolower(static_cast<unsigned char>(c)));
                    int td = 0, lom = 0, eas = 0;
                    if (s == "td") td = 1;
                    else if (s == "tdnolpyear") td = 2;
                    else if (s == "tdstock") td = 3;
                    else if (s == "td1coef") td = 4;
                    else if (s == "td1nolpyear") td = 5;
                    else if (s == "tdstock1coef") td = 6;
                    else if (s == "easter") eas = 1;
                    else if (s == "easterstock") eas = 2;
                    else if (s == "lom") lom = 1;
                    else if (s == "loq") lom = 2;
                    else if (s == "lpyear") lom = 3;
                    if (td > 0) {
                        if (A.itdtst != 0) {
                            inpter(ctx, PERROR, aep,
                                   "Can only specify one type of trading day in "
                                   "aictest."); inptok = false;
                        } else A.itdtst = td;
                    } else if (eas > 0) {
                        if (A.leastr && A.eastst != 0) {
                            inpter(ctx, PERROR, aep,
                                   "Can only specify one of easter and easterstock "
                                   "in aictest."); inptok = false;
                        } else { A.leastr = true; A.eastst = eas; }
                    } else if (lom > 0) {
                        if (A.lomtst != 0) {
                            inpter(ctx, PERROR, aep,
                                   "Can only specify one of lom, loq, or lpyear in "
                                   "aictest."); inptok = false;
                        } else A.lomtst = lom;
                    } else if (s == "user") {
                        A.luser = true;
                    }
                }
                if (!tmp.empty()) ctx.model.iregfx = 0;  // getreg.f:276
            }
        }
    }

    // getreg.f:519-553 -- the b= writeback. It runs HERE, straight after the
    // argument loop, because Nb is only final once every variables= group has
    // been built by gtpdrg, and it runs BEFORE the user-column adrgef calls
    // below on purpose: those read B(idisp)/Regfx(idisp) out of the slots this
    // loop fills past Nb, which is how a user regressor gets an initial value.
    if (nbvec != prm::NOTSET) {
        model_cmn& M = ctx.model;
        // Insert a value for the Leap Year regressor that will be removed
        // later (rmlnvr), so a b= list that omits it still lines up with Nb.
        if (ctx.picktd.picktd &&
            (ctx.arima.fcntyp != 4 && !dpeq(ctx.arima.lam, 1.0))) {
            int ic1 = 1;
            int icol = strinx(true, M.colttl.raw(), M.colptr.data(), ic1, M.nb,
                              "Leap Year");
            while (icol > 0) {
                if (icol <= nbvec) {
                    for (int i = nbvec; i >= icol; --i) {
                        if (i + 1 <= prm::PB) {
                            bvec[i] = bvec[i - 1];
                            fixvec[i] = fixvec[i - 1];
                        }
                    }
                }
                // PORTED VERBATIM: only bvec gets the spliced value. fixvec
                // was shifted up but its slot at icol is NOT reset, so the
                // Leap Year column inherits the fix flag of whatever used to
                // sit there. Checked, and it is NOT a Census bug worth logging:
                // the inherited flag always equals some other column's flag
                // that regfix is ANDing in anyway (so it cannot independently
                // change allfix), and rmlnvr (gtinpt.f:1032) removes the
                // column before rmfix could ever act on it.
                if (icol >= 1 && icol <= prm::PB) bvec[icol - 1] = 1.0;
                ++nbvec;
                if (icol == M.nb) {
                    icol = 0;
                } else {
                    ic1 = icol + 1;
                    icol = strinx(true, M.colttl.raw(), M.colptr.data(), ic1,
                                  M.nb, "Leap Year");
                }
            }
        }
        const int ntot = M.nb + ctx.usrreg.ncusrx;
        if (nbvec > 0 && nbvec != ntot) {
            // getreg.f:543-549 writes "ERROR: Number of initial values is not
            // the same as the number of regression variables." to STDERR/Mt2
            // with a plain WRITE -- NOT via inpter -- so locok is untouched
            // and the run continues with NO coefficients applied. The message
            // is print surface; the skipped writeback is the behaviour.
        } else {
            for (int i = 1; i <= ntot && i <= prm::PB; ++i) {
                M.regfx(i) = fixvec[i - 1];
                ctx.mdldat.b(i) = bvec[i - 1];
            }
        }
    }

    // getreg.f:558-567 -- if data comes from a file, load it now.
    if (hvfile && !haveux) {
        if (ctx.usrreg.ncusrx > 0) {
            if (havfmt) {
                inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                       "formatted user-regressor files (format=) are not yet "
                       "supported; use free-format data.");
                inptok = false;
            } else {
                bool hvfreq = false; int freq = 0; bool argok = true;
                gtfldt_free(ctx, prm::PUSERX, rgfile, nflchr,
                            ctx.arima.userx.data(), neltux, hvfreq, freq, hvstrt,
                            argok, inptok);
                if (ctx.error.lfatal) return;
                haveux = argok && neltux > 0;
            }
        }
    }
    (void)nfmtch;

    // getreg.f:571-736 tail -- attach the user-defined regressor groups.
    ctx.usrreg.usrttl = usrttl;   // persist the packed column names.
    if (hvuttl || haveux) {
        const int* ep = ctx.lex.errpos.data() + 1;
        const int ncusrx = ctx.usrreg.ncusrx;
        if (hvuttl != haveux) {
            inpter(ctx, PERROR, ep,
                   "Need to specify both user-defined regression variables (with "
                   "user argument) and X matrix (with file or data argument).");
            inptok = false;
        } else if (ncusrx > 0 && (neltux % ncusrx) != 0) {
            inpter(ctx, PERROR, ep,
                   "Number of user-defined X elements not equal to a multiple of "
                   "the number of columns.");
            inptok = false;
        } else {
            if (!hvstrt) {
                ctx.arima.bgusrx(1) = ctx.arima.begsrs(1);
                ctx.arima.bgusrx(2) = ctx.arima.begsrs(2);
            }
            ctx.arima.nrusrx = neltux / ncusrx;
            // getreg.f:576-598 -- broadcast a single usertype, then check the
            // holiday-group sequence.
            if (nusrrg > 0) {
                if (nusrrg == 1)
                    for (int i = 2; i <= ncusrx; ++i)
                        ctx.usrreg.usrtyp(i) = ctx.usrreg.usrtyp(1);
                bool herror = false;
                chkuhg(iuhl, ctx.usrreg.nguhl, herror);
                if (herror) {
                    inpter(ctx, PERROR, ep,
                           "Cannot specify holiday group types for user-defined "
                           "regression variables out of sequence.");
                    inptok = false;
                }
            }
            if (!chkcvr(ctx.arima.bgusrx.data(), ctx.arima.nrusrx,
                        ctx.mdldat.begspn.data(), ctx.mdldat.nspobs, ctx.model.sp)) {
                inpter(ctx, PERROR, ep,
                       "user-defined regression variables do not cover the span "
                       "of the data.");
                inptok = false;
            } else {
                // getreg.f:627 -- idisp walks the slots the b= writeback above
                // filled past Nb, so a user regressor's initial value / fix
                // flag come from B(idisp)/Regfx(idisp), not from zero.
                int idisp = ctx.model.grp(ctx.model.ngrp) - 1;
                for (int i = 1; i <= ncusrx; ++i) {
                    ++idisp;
                    std::string effttl; int nchr = 0;
                    getstr(ctx, ctx.usrreg.usrttl.data(),
                           ctx.usrreg.usrptr.data(), ncusrx, i, effttl, nchr);
                    if (ctx.error.lfatal) return;
                    std::string_view et = std::string_view(effttl).substr(
                        0, static_cast<std::size_t>(nchr));
                    // getreg.f:631-685 -- title/type per column's usrtyp.
                    const int ut = ctx.usrreg.usrtyp(i);
                    const char* gt = nullptr; int vt = 0;
                    switch (ut) {
                    case prm::PRGTUS: gt = "User-defined Seasonal";     vt = prm::PRGTUS; break;
                    case prm::PRGUCN: gt = "User-defined Constant";     vt = prm::PRGUCN; break;
                    case prm::PRGUTD: gt = "User-defined Trading Day";  vt = prm::PRGUTD; break;
                    case prm::PRGULM: gt = "User-defined LOM";          vt = prm::PRGULM; break;
                    case prm::PRGULQ: gt = "User-defined LOQ";          vt = prm::PRGULQ; break;
                    case prm::PRGULY: gt = "User-defined Leap Year";    vt = prm::PRGULY; break;
                    case prm::PRGUAO: gt = "User-defined AO";           vt = prm::PRGUAO; break;
                    case prm::PRGULS: gt = "User-defined LS";           vt = prm::PRGULS; break;
                    case prm::PRGUSO: gt = "User-defined SO";           vt = prm::PRGUSO; break;
                    case prm::PRGUCY: gt = "User-defined Transitory";   vt = prm::PRGUCY; break;
                    case prm::PRGTUH: gt = "User-defined Holiday";          vt = prm::PRGTUH; break;
                    case prm::PRGUH2: gt = "User-defined Holiday Group 2";  vt = prm::PRGUH2; break;
                    case prm::PRGUH3: gt = "User-defined Holiday Group 3";  vt = prm::PRGUH3; break;
                    case prm::PRGUH4: gt = "User-defined Holiday Group 4";  vt = prm::PRGUH4; break;
                    case prm::PRGUH5: gt = "User-defined Holiday Group 5";  vt = prm::PRGUH5; break;
                    case 0:
                    case prm::PRGTUD: gt = "User-defined";              vt = prm::PRGTUD; break;
                    default:
                        inpter(ctx, PERROR, ep,
                               "unsupported user-defined regressor type.");
                        inptok = false;
                        return;
                    }
                    // Copy out before the call: adrgef writes B(icol)=initvl,
                    // and in the Fortran initvl IS B(idisp) (pass by
                    // reference) -- a self-assignment there, but the C++ must
                    // not alias the argument with the array it writes.
                    const double initvl = (idisp >= 1 && idisp <= prm::PB)
                                              ? ctx.mdldat.b(idisp) : 0.0;
                    const bool varfix = (idisp >= 1 && idisp <= prm::PB)
                                            ? ctx.model.regfx(idisp) : false;
                    adrgef(ctx, initvl, et, gt, vt, varfix, true);
                    if (ctx.error.lfatal) return;
                }
                // getreg.f:692-732 -- remove regressor or seasonal mean.
                double* ux = ctx.arima.userx.data();
                if (lumean) {
                    std::vector<double> urmean(static_cast<std::size_t>(ncusrx), 0.0);
                    for (int i = 1; i <= neltux; ++i) {
                        int i2 = i % ncusrx; if (i2 == 0) i2 = ncusrx;
                        urmean[i2 - 1] += ux[i - 1];
                    }
                    for (int c = 0; c < ncusrx; ++c)
                        urmean[c] /= static_cast<double>(ctx.arima.nrusrx);
                    for (int i = 1; i <= neltux; ++i) {
                        int i2 = i % ncusrx; if (i2 == 0) i2 = ncusrx;
                        ux[i - 1] -= urmean[i2 - 1];
                    }
                } else if (luseas) {
                    const int sp = ctx.model.sp;
                    const int n2 = sp * ncusrx;
                    for (int i = 1; i <= sp; ++i) {
                        std::vector<double> urmean(static_cast<std::size_t>(ncusrx), 0.0);
                        std::vector<double> urnum(static_cast<std::size_t>(ncusrx), 0.0);
                        int i2 = (i - 1) * ncusrx + 1;
                        for (int j = i2; j <= neltux; j += n2)
                            for (int k = j; k <= ncusrx + j - 1; ++k) {
                                int k2 = k % ncusrx; if (k2 == 0) k2 = ncusrx;
                                urmean[k2 - 1] += ux[k - 1];
                                urnum[k2 - 1] += 1.0;
                            }
                        for (int c = 0; c < ncusrx; ++c) urmean[c] /= urnum[c];
                        for (int j = i2; j <= neltux; j += n2)
                            for (int k = j; k <= ncusrx + j - 1; ++k) {
                                int k2 = k % ncusrx; if (k2 == 0) k2 = ncusrx;
                                ux[k - 1] -= urmean[k2 - 1];
                            }
                    }
                }
            }
        }
    }

    // getreg.f:738-766 -- a SIBLING of the two blocks above (same nesting
    // level), so it runs whether or not there are user regressors.
    if (ctx.model.nb > 0) {
        // Derive Iregfx from the b= values just written. arima.f:282 reads it.
        regfix(ctx);
        // getreg.f:746-767 -- Userfx: "at least one FIXED user-defined column".
        // With everything fixed (Iregfx==3) it is unconditional; otherwise walk
        // the groups and OR the Regfx of every user-typed column. rmfix/addfix
        // read it to decide whether the dlusrg/addusr special case applies.
        model_cmn& M = ctx.model;
        M.userfx = false;
        if (ctx.usrreg.ncusrx > 0 && M.iregfx >= 2) {
            if (M.iregfx == 3) {
                M.userfx = true;
            } else {
                for (int igrp = 1; igrp <= M.ngrp; ++igrp) {
                    const int begcol = M.grp(igrp - 1);
                    const int endcol = M.grp(igrp) - 1;
                    const int rtype = M.rgvrtp(begcol);
                    if (rtype == prm::PRGTUD || rtype == prm::PRGTUS ||
                        rtype == prm::PRGTUH || rtype == prm::PRGUH2 ||
                        rtype == prm::PRGUH3 || rtype == prm::PRGUH4 ||
                        rtype == prm::PRGUH5 || rtype == prm::PRGUAO ||
                        rtype == prm::PRGULS || rtype == prm::PRGUSO ||
                        rtype == prm::PRGUCN || rtype == prm::PRGUCY ||
                        rtype == prm::PRGUTD || rtype == prm::PRGULM ||
                        rtype == prm::PRGULQ || rtype == prm::PRGULY) {
                        for (int i = begcol; i <= endcol; ++i)
                            M.userfx = M.userfx || M.regfx(i);
                    }
                }
            }
        }
        // getreg.f:771 otsort() (sorting user-specified outlier regressors into
        // date order) is not ported; the corpus specifies outliers in order.
    }
}

// ---- automdl{} (gtauto.f) --------------------------------------------------
void gt_automdl(X13Context& ctx, bool& inptok) {
    constexpr int PARG = 24;
    static const char ARGDIC[] =
        "maxdiffub1ub2cancelmaxorderdiffprintsavelogbalancedexactdiffhrinitial"
        "armalimitpercentrsereducecvljungboxlimitacceptdefaultnoautooutlier"
        "urfinalfirstarcheckmumixedrejectfcstfcstlimseasonaloverdiff";
    static const int argptr[PARG + 1] = {1, 8, 11, 14, 20, 28, 32, 37, 44, 52, 61,
        70, 79, 89, 97, 110, 123, 136, 143, 150, 157, 162, 172, 179, 195};
    // gt_generic-equivalent token consumption, but capture acceptdefault (arg 16
    // in ARGDIC -- ljungboxlimit is 15; gtauto.f:391-397 -> Laccdf) so automd's
    // accept-default-model path becomes reachable. Every other arg stays token-
    // consumed/deferred exactly as before.
    int arglog0[2 * PARG];
    for (auto& v : arglog0) v = -32767;  // NOTSET
    int a_idx;
    while (gtarg(ctx, ARGDIC, argptr, PARG, a_idx, arglog0, inptok)) {
        if (ctx.error.lfatal) return;
        std::vector<std::string> cap;
        const bool want = (a_idx == 16);  // acceptdefault
        consume_value(ctx, want ? &cap : nullptr);
        if (ctx.error.lfatal) return;
        if (a_idx == 16 && !cap.empty())
            ctx.arima.laccdf = (cap[0] == "yes");  // gtauto.f:397 Laccdf=ivec(1).eq.1
    }

    // gtauto.f tail defaults (gtauto.f 491-510). The maxorder/maxdiff/ub/... arg
    // VALUES are still token-consumed by gt_generic without application; here we
    // apply the no-argument defaults the automatic-model driver needs. Maxord and
    // Diffam are left at their gtinpt sentinels (0,0)/(NOTSET) until an arg sets
    // them, so "still sentinel" == "unspecified".
    auto& ar = ctx.arima;
    ar.lautom = true;                       // automatic model selection on
    ar.lautod = true;                       // and automatic differencing (no diff arg)
    if (ar.maxord(1) == 0 && ar.maxord(2) == 0) {
        ar.maxord(1) = 2;
        ar.maxord(2) = 1;
    }
    if (ar.diffam(1) == prm::NOTSET) {
        ar.diffam(1) = 2;
        ar.diffam(2) = 1;
    }
}

// ---- estimate{} (gtestm.f) -------------------------------------------------
// gtestm.f: applies the estimation-numeric knobs (maxiter/maxnliter/tol/nltol/
// parms/exact/step) to the model+arima commons; the output/AIC/model-file args
// (outofsample/print/save/savelog/file/fix/k/removeconstant) are token-consumed
// with their state application deferred to their own milestones. The tolerance-
// reconciliation tail (gtestm.f 285-292) is reproduced.
void gt_estimate(X13Context& ctx, bool& inptok) {
    constexpr int PARG = 15;
    static const char ARGDIC[] =
        "maxitermaxnlitertolnltolparmsexactoutofsampleprintsavesavelogfile"
        "fixstepkremoveconstant";
    static const int argptr[PARG + 1] = {1, 8, 17, 20, 25, 30, 35, 46, 51, 55, 62,
        66, 69, 73, 74, 88};
    static const char ESTDIC[] = "fixedestimated";   // parms choices
    static const int estptr[3] = {1, 6, 15};
    static const char EXTDIC[] = "armamanone";        // exact choices
    static const int extptr[4] = {1, 5, 7, 11};

    auto& m = ctx.model;
    const double mprec = dpmpar(1);
    int arglog[2 * PARG];
    for (auto& v : arglog) v = -32767;   // NOTSET
    bool hvtol = false, hvnltl = false;
    int argidx;
    while (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
        if (ctx.error.lfatal) return;
        const int* ep = ctx.lex.errpos.data() + 1;
        int ivec[1]; double dvec[1]; int nelt = 0; bool argok = true;
        switch (argidx) {
        case 1:   // maxiter -- maximum overall iterations
            getivc(ctx, LPAREN, true, 1, ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            ctx.arima.mxiter = ivec[0];
            break;
        case 2:   // maxnliter -- maximum nonlinear iterations
            getivc(ctx, LPAREN, true, 1, ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            ctx.arima.mxnlit = ivec[0];
            break;
        case 3:   // tol -- overall convergence tolerance
            gtdpvc(ctx, LPAREN, true, 1, dvec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            m.tol = dvec[0];
            if (nelt > 0) {
                if (1.0 / ctx.mdldat.nspobs * m.tol < mprec) {
                    inpter(ctx, PERROR, ep,
                           "Overall tolerance is smaller than machine precision");
                    hvtol = false;
                    inptok = false;
                } else {
                    hvtol = true;
                }
            }
            break;
        case 4:   // nltol -- nonlinear convergence tolerance
            gtdpvc(ctx, LPAREN, true, 1, dvec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (nelt > 0) {
                m.nltol = dvec[0];
                if (m.nltol < 100.0 * mprec) {
                    inpter(ctx, PERROR, ep,
                           "Nonlinear tolerance is smaller than machine precision");
                    inptok = false;
                    hvnltl = false;
                } else {
                    hvnltl = true;
                }
            }
            break;
        case 5:   // parms -- fixed | estimated
            gtdcvc(ctx, LPAREN, true, 1, ESTDIC, estptr, 2,
                   "Choices are fixed or estimated", ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (nelt > 0) {
                if (ivec[0] != 1) {
                    ctx.arima.lestim = true;
                } else if (m.imdlfx >= 1) {
                    ctx.arima.lestim = false;
                } else {
                    inpter(ctx, PERROR, ep,
                           "Must specify all ARMA parameters to evaluate");
                    inptok = false;
                }
            }
            break;
        case 6:   // exact -- arma | ma | none (conditional)
            gtdcvc(ctx, LPAREN, true, 1, EXTDIC, extptr, 3,
                   "Choices are ARMA, MA, or NONE (conditional)", ivec, nelt,
                   argok, inptok);
            if (ctx.error.lfatal) return;
            if (nelt > 0) {
                if (ivec[0] == 1) { m.lextar = true; m.lextma = true; }
                else if (ivec[0] == 2) { m.lextar = false; m.lextma = true; }
                else if (ivec[0] == 3) { m.lextar = false; m.lextma = false; }
            }
            break;
        case 13:  // step -- numerical-derivative step size
            gtdpvc(ctx, LPAREN, true, 1, dvec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (nelt > 0) {
                m.stepln = dvec[0];
                if (m.stepln < 0.0) {
                    inpter(ctx, PERROR, ep,
                           "Step size of numerical derivatives cannot be less "
                           "than zero.");
                    inptok = false;
                }
            }
            break;
        default:  // 7 outofsample, 8 print, 9 save, 10 savelog, 11 file, 12 fix,
                  // 14 k (EIC penalty), 15 removeconstant -- state application
                  // (output tables / model file / AIC) deferred to their
                  // milestones; consume the value grammar to stay token-faithful.
            consume_value(ctx, nullptr);
            break;
        }
        if (ctx.error.lfatal) return;
    }
    if (ctx.error.lfatal) return;
    // gtestm.f 285-292: set the ARMA + initial-ARMA tolerances from whichever
    // of tol / nltol was actually supplied.
    if (hvtol && !hvnltl) {
        m.nltol = m.tol;
        m.nltol0 = 100.0 * m.tol;
    } else if (hvnltl) {
        m.nltol0 = m.nltol;
    }
}

// ---- outlier{} (gtotlr.f) --------------------------------------------------
void gt_outlier(X13Context& ctx, bool& inptok) {
    constexpr int PARG = 12;
    static const char ARGDIC[] =
        "typesmethodcriticallsrunspanprintsavetcratecriticalalphadefault"
        "criticalalmostsavelog";
    static const int argptr[PARG + 1] = {1, 6, 12, 20, 25, 29, 34, 38, 44, 57, 72, 78, 85};

    // gtotlr.f defaults: identify AO+LS by ADDONE, critical value from the span.
    ctx.arima.ltstao = true;
    ctx.arima.ltstls = true;
    ctx.arima.ltsttc = false;
    ctx.arima.ladd1 = true;
    ctx.captured.has_outlier = true;
    const int sp = ctx.model.sp;

    int arglog[2 * PARG];
    for (auto& v : arglog) v = -32767;
    int argidx;
    while (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
        if (ctx.error.lfatal) return;
        // Capture value tokens for the args we act on: 1 types, 2 method,
        // 3 critical, 9 criticalalpha.
        bool want = (argidx == 1 || argidx == 2 || argidx == 3 || argidx == 9);
        std::vector<std::string> cap;
        consume_value(ctx, want ? &cap : nullptr);
        if (ctx.error.lfatal) return;
        if (argidx == 1 && !cap.empty()) {           // types
            ctx.arima.ltstao = false;
            ctx.arima.ltstls = false;
            ctx.arima.ltsttc = false;
            for (auto& t : cap) {
                if (t == "ao") ctx.arima.ltstao = true;
                else if (t == "ls") ctx.arima.ltstls = true;
                else if (t == "tc") ctx.arima.ltsttc = true;
                else if (t == "all") {
                    ctx.arima.ltstao = true;
                    ctx.arima.ltstls = true;
                    if (sp >= 4) ctx.arima.ltsttc = true;
                }
                // "none" leaves all false.
            }
        } else if (argidx == 2 && !cap.empty()) {    // method: addone|addall
            ctx.arima.ladd1 = (cap[0] == "addone");
        } else if (argidx == 3 && !cap.empty()) {    // critical value(s)
            if (cap.size() == 1) {
                double v = 0;
                try { v = std::stod(cap[0]); } catch (...) { v = 0; }
                for (int i = 1; i <= prm::POTLR; ++i) ctx.arima.critvl(i) = v;
            } else {
                for (std::size_t i = 0; i < cap.size() && i < prm::POTLR; ++i) {
                    try { ctx.arima.critvl(static_cast<int>(i) + 1) = std::stod(cap[i]); }
                    catch (...) {}
                }
            }
        } else if (argidx == 9 && !cap.empty()) {    // criticalalpha
            try { ctx.arima.cvalfa = std::stod(cap[0]); } catch (...) {}
        }
    }
}

// ---- seats{} (gtseat.f) -----------------------------------------------------
// Argument dictionary order (== the oracle's GOTO(20,30,40,50,60,70,90,100,
// 110,120,130,140,150,160,170,190,200,240,260,270,280,290),argidx computed
// jump table -- the missing labels 80/180/210-230/250 have no matching
// dictionary entries, so ARGDIC's 22 tokens map 1:1 onto argidx 1..22):
//   1 print  2 save  3 savelog  4 appendfcst  5 noadmiss  6 imean  7 qmax
//   8 out  9 epsphi  10 xl  11 rmod  12 epsiv  13 maxit  14 hplan
//   15 hpcycle  16 statseas  17 tabtables  18 bias  19 finite
//   20 printphtrf  21 hptarget  22 hprmls
void gt_seats(X13Context& ctx, bool& inptok) {
    constexpr int PARG = 22;
    static const char ARGDIC[] =
        "printsavesavelogappendfcstnoadmissimeanqmaxoutepsphixlrmodepsiv"
        "maxithplanhpcyclestatseastabtablesbiasfiniteprintphtrfhptargethprmls";
    static const int argptr[PARG + 1] = {1, 6, 10, 17, 27, 35, 40, 44, 47, 53, 55,
        59, 64, 69, 74, 81, 89, 98, 102, 108, 118, 126, 132};

    // gtinpt.f:533-550 defaults. Set here (only observed when seats{} is
    // present -- nothing else in the port reads ctx.seatop) rather than at
    // global spec-parse start like the oracle, which is behaviorally
    // equivalent for a value nothing consumes before this point.
    seatop_cmn& o = ctx.seatop;
    o.lnoadm = false;
    o.kmean = prm::NOTSET;
    o.lstsea = false;
    o.lhp = true;
    o.lfinit = false;
    o.lhprmls = false;
    o.qmax2 = prm::NOTSET;
    o.out2 = prm::NOTSET;
    o.maxit2 = prm::NOTSET;
    o.epsph2 = prm::DNOTST;
    o.xl2 = prm::DNOTST;
    o.rmod2 = prm::DNOTST;
    o.epsiv2 = prm::DNOTST;
    o.hplan2 = prm::DNOTST;
    o.bias2 = prm::NOTSET;
    o.iphtrf = prm::NOTSET;
    o.hptrgt = prm::NOTSET;

    int arglog[2 * PARG];
    for (auto& v : arglog) v = -32767;
    int argidx;
    while (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
        if (ctx.error.lfatal) return;
        const int* ep = ctx.lex.errpos.data() + 1;
        std::vector<std::string> cap;
        // 2=save captured for the shared save-table list; 17=tabtables is a
        // deferred (tier-F output) name list, consumed but not stored;
        // everything else that carries a value is captured for parsing below.
        bool want = argidx != 17;
        consume_value(ctx, want ? &cap : nullptr);
        if (ctx.error.lfatal) return;
        if (cap.empty()) continue;
        const std::string& v0 = cap[0];
        auto yes = [&]() { return v0 == "yes"; };
        switch (argidx) {
        case 2:   // save
            for (const auto& t : cap) ctx.captured.save_tables.push_back(t);
            break;
        case 4:   // appendfcst -- gtseat.f:103-108 (Savfct=ivec(1)==1)
            ctx.tbllog.savfct = yes();
            break;
        case 5:   // noadmiss -- gtseat.f:112-117
            o.lnoadm = yes();
            break;
        case 6:   // imean -- gtseat.f:121-126 (Kmean=2-ivec(1); yes->1,no->2)
            o.kmean = yes() ? 1 : 0;
            break;
        case 7: {  // qmax -- gtseat.f:130-143 (>=0)
            int v = 0; bool ok = true;
            try { v = std::stoi(v0); } catch (...) { ok = false; }
            if (ok) {
                if (v < 0) {
                    inpter(ctx, PERROR, ep, "Limit for Ljung-Box Q must be > 0.");
                } else {
                    o.qmax2 = v;
                }
            } else {
                inpter(ctx, PERROR, ep, "Invalid limit for Ljung-Box Q.");
            }
            break;
        }
        case 8: {  // out -- gtseat.f:147-160 (0,1,2)
            int v = 0; bool ok = true;
            try { v = std::stoi(v0); } catch (...) { ok = false; }
            if (ok && v >= 0 && v <= 2) {
                o.out2 = v;
            } else {
                inpter(ctx, PERROR, ep, "Out must be either 0, 1, or 2.");
            }
            break;
        }
        case 9: {  // epsphi -- gtseat.f:165-176 (>=0)
            double v = 0.0; bool ok = true;
            try { v = std::stod(v0); } catch (...) { ok = false; }
            if (ok && v >= 0.0) {
                o.epsph2 = v;
            } else {
                inpter(ctx, PERROR, ep,
                       "Epsphi must be greater than or equal to zero.");
            }
            break;
        }
        case 10: {  // xl -- gtseat.f:180-192 ([0.5,1])
            double v = 0.0; bool ok = true;
            try { v = std::stod(v0); } catch (...) { ok = false; }
            if (ok && v >= 0.5 && v <= 1.0) {
                o.xl2 = v;
            } else {
                inpter(ctx, PERROR, ep,
                       "Xl must be greater than or equal to 0.5 and less than "
                       "or equal to one.");
            }
            break;
        }
        case 11: {  // rmod -- gtseat.f:196-208 ([0,1])
            double v = 0.0; bool ok = true;
            try { v = std::stod(v0); } catch (...) { ok = false; }
            if (ok && v >= 0.0 && v <= 1.0) {
                o.rmod2 = v;
            } else {
                inpter(ctx, PERROR, ep,
                       "Rmod must be greater than or equal to zero and less "
                       "than or equal to one.");
            }
            break;
        }
        case 12: {  // epsiv -- gtseat.f:212-222 (>0)
            double v = 0.0; bool ok = true;
            try { v = std::stod(v0); } catch (...) { ok = false; }
            if (ok && v > 0.0) {
                o.epsiv2 = v;
            } else {
                inpter(ctx, PERROR, ep, "Epsiv must be greater than zero.");
            }
            break;
        }
        case 13: {  // maxit -- gtseat.f:226-239 (>0)
            int v = 0; bool ok = true;
            try { v = std::stoi(v0); } catch (...) { ok = false; }
            if (ok && v > 0) {
                o.maxit2 = v;
            } else {
                inpter(ctx, PERROR, ep, "Value for maxit must be > 0.");
            }
            break;
        }
        case 14: {  // hplan -- gtseat.f:243-253 (>0)
            double v = 0.0; bool ok = true;
            try { v = std::stod(v0); } catch (...) { ok = false; }
            if (ok && v > 0.0) {
                o.hplan2 = v;
            } else {
                inpter(ctx, PERROR, ep, "Hplan must be greater than zero.");
            }
            break;
        }
        case 15:  // hpcycle -- gtseat.f:257-262
            o.lhp = yes();
            break;
        case 16:  // statseas -- gtseat.f:266-271
            o.lstsea = yes();
            break;
        case 18: {  // bias -- gtseat.f:281-295 (-1,0,1)
            int v = 0; bool ok = true;
            try { v = std::stoi(v0); } catch (...) { ok = false; }
            if (ok && v >= -1 && v <= 1) {
                o.bias2 = v;
            } else {
                inpter(ctx, PERROR, ep, "Bias must be either -1, 0, or 1.");
            }
            break;
        }
        case 19:  // finite -- gtseat.f:299-304
            o.lfinit = yes();
            break;
        case 20: {  // printphtrf -- gtseat.f:308-322 (0 or 1)
            int v = 0; bool ok = true;
            try { v = std::stoi(v0); } catch (...) { ok = false; }
            if (ok && v >= 0 && v <= 1) {
                o.iphtrf = v;
            } else {
                inpter(ctx, PERROR, ep, "printphtrf must be either 0 or 1.");
            }
            break;
        }
        case 21:  // hptarget -- gtseat.f:326-330 (HPTDIC='trendsadjorig')
            if (v0 == "trend") o.hptrgt = 1;
            else if (v0 == "sadj") o.hptrgt = 2;
            else if (v0 == "orig") o.hptrgt = 3;
            break;
        case 22:  // hprmls -- gtseat.f:335-339
            o.lhprmls = yes();
            break;
        default:  // 1 print, 3 savelog -- table/log selection deferred
            break;
        }
        if (ctx.error.lfatal) return;
    }
    ctx.captured.has_seats = true;
}

// force{} `start` accepts a month name (jan..dec, january..december -> 1..24)
// or a quarter name (q1..q4 -> 25..28), matching getfrc.f's SUMDIC. Returns the
// 1-based dictionary index, or 0 if unrecognized.
static int force_start_index(const std::string& v) {
    static const char* const kNames[28] = {
        "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct",
        "nov", "dec", "january", "february", "march", "april", "may", "june",
        "july", "august", "september", "october", "november", "december",
        "q1", "q2", "q3", "q4"};
    for (int i = 0; i < 28; ++i)
        if (v == kNames[i]) return i + 1;
    return 0;
}

// ---- force{} (getfrc.f) ----------------------------------------------------
void gt_force(X13Context& ctx, bool& inptok) {
    constexpr int PARG = 11;
    static const char ARGDIC[] =
        "typeroundtargetstartlambdarhomodeprintsaveindforceusefcst";
    static const int argptr[PARG + 1] =
        {1, 5, 10, 16, 21, 27, 30, 34, 39, 43, 51, 58};

    // Categorical dictionaries (getfrc.f:40-88). Routing type/round/target/mode/
    // indforce/usefcst through gtdcvc (rather than a hand-rolled string compare)
    // matches the oracle's exact token consumption: an invalid value leaves the
    // parser positioned so the dispatch loop then reports the paired
    // "Argument name X not found" line -- the 2nd cascading error the oracle emits.
    static const char FRCDIC[] = "nonedentonregress";        // type
    static const int frcptr[4] = {1, 5, 11, 18};
    static const char FRTDIC[] = "originalcalendaradjpermprioradjboth";  // target
    static const int frtptr[5] = {1, 9, 20, 32, 36};
    static const char FMDDIC[] = "ratiodiff";                // mode
    static const int fmdptr[3] = {1, 6, 10};
    static const char YSNDIC[] = "yesno";                    // round/indforce/usefcst
    static const int ysnptr[3] = {1, 4, 6};

    // getfrc.f entry defaults (set by the caller in the oracle; nothing reads
    // ctx.force before force{} is parsed, so setting them here is equivalent).
    force_cmn& f = ctx.force;
    f.iyrt = prm::NOTSET;
    f.lrndsa = false;
    f.iftrgt = prm::NOTSET;
    f.begyrt = prm::NOTSET;
    f.mid = prm::NOTSET;
    f.lamda = prm::DNOTST;
    f.rol = prm::DNOTST;
    f.lindfr = true;   // gtinpt.f:426 default
    f.lfctfr = true;   // gtinpt.f:427 default (force over the forecast-extended span)

    const int sp = ctx.model.sp;
    const bool havesp = (sp == 4 || sp == 12);

    int arglog[2 * PARG];
    for (auto& v : arglog) v = -32767;
    int argidx;
    while (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
        if (ctx.error.lfatal) return;
        const int* ep = ctx.lex.errpos.data() + 1;
        int ivec[1]; int nelt = 0; bool argok = true;
        switch (argidx) {
        case 1:  // type: none|denton|regress -> Iyrt 0/1/2 (getfrc.f:102)
            gtdcvc(ctx, LPAREN, true, 1, FRCDIC, frcptr, 3,
                   "Entry for type argument must be none, denton or regress.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (nelt > 0) f.iyrt = ivec[0] - 1;
            break;
        case 2:  // round: yes|no (getfrc.f:111)
            gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                   "Available options for round are yes or no.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (nelt > 0) f.lrndsa = (ivec[0] == 1);
            break;
        case 3:  // target: original|calendaradj|permprioradj|both -> 0..3 (getfrc.f:120)
            gtdcvc(ctx, LPAREN, true, 1, FRTDIC, frtptr, 4,
                   "Entry for forcetarget argument must be original, "
                   "calendaradj, permprioradj, or both.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (nelt > 0) f.iftrgt = ivec[0] - 1;
            break;
        case 7:  // mode: ratio|diff -> Mid 0/1 (getfrc.f:216)
            gtdcvc(ctx, LPAREN, true, 1, FMDDIC, fmdptr, 2,
                   "Available options for mode are ratio or diff.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (nelt > 0) f.mid = ivec[0] - 1;
            break;
        case 10:  // indforce: yes|no (getfrc.f:162)
            gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                   "Available options for indforce are yes or no.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (nelt > 0) f.lindfr = (ivec[0] == 1);
            break;
        case 11:  // usefcst: yes|no (getfrc.f:171)
            gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                   "Available options for usefcst are yes or no.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (nelt > 0) f.lfctfr = (ivec[0] == 1);
            break;
        default: {
            // Value-token args (not dictionary categoricals): start(4), lambda(5),
            // rho(6), print(8, deferred), save(9). Capture the value token(s).
            std::vector<std::string> cap;
            const bool want = argidx != 8;  // 8=print: consume without storing
            consume_value(ctx, want ? &cap : nullptr);
            if (ctx.error.lfatal) return;
            if (cap.empty()) break;
            const std::string& v0 = cap[0];
            switch (argidx) {
            case 4: {  // start: month/quarter name -> Begyrt
                if (!havesp) {
                    inpter(ctx, PERROR, ep,
                           "No seasonal period specified in series spec.");
                    inptok = false;
                    break;
                }
                const int idx = force_start_index(v0);
                if (idx >= 1 && idx <= 24 && sp == 12) {
                    f.begyrt = (idx > 12) ? idx - 12 : idx;
                } else if (idx >= 25 && idx <= 28 && sp == 4) {
                    f.begyrt = idx - 24;
                } else {
                    inpter(ctx, PERROR, ep,
                           sp == 12
                               ? "This entry for start only valid for monthly data."
                               : "This entry for start only valid for quarterly "
                                 "data.");
                    inptok = false;
                }
                break;
            }
            case 5: {  // lambda: [-3,3]
                double v = 0.0; bool ok = true;
                try { v = std::stod(v0); } catch (...) { ok = false; }
                if (ok && v >= -3.0 && v <= 3.0) {
                    f.lamda = v;
                } else {
                    inpter(ctx, PERROR, ep,
                           "Value of lambda must be between -3 and 3.");
                    inptok = false;
                }
                break;
            }
            case 6: {  // rho: (0,1] (getfrc.f:203-210: reject v<0, v>1, or dpeq(v,0))
                double v = 0.0; bool ok = true;
                try { v = std::stod(v0); } catch (...) { ok = false; }
                if (ok && !(v < 0.0 || v > 1.0 || dpeq(v, 0.0))) {
                    f.rol = v;
                } else {
                    inpter(ctx, PERROR, ep,
                           "Value of rho must be greater than 0 and less than or "
                           "equal to 1.");
                    inptok = false;
                }
                break;
            }
            case 9:  // save
                for (const auto& t : cap) ctx.captured.save_tables.push_back(t);
                break;
            default:  // 8 print -- deferred table selection
                break;
            }
            break;
        }
        }
        if (ctx.error.lfatal) return;
    }

    // Defaults / inference (getfrc.f:236-260).
    if (f.iyrt == prm::NOTSET) {
        if (f.lamda != prm::DNOTST || f.rol != prm::DNOTST ||
            f.mid != prm::NOTSET) {
            f.iyrt = 2;
        } else if (f.begyrt != prm::NOTSET || f.iftrgt != prm::NOTSET) {
            f.iyrt = 1;
        } else {
            f.iyrt = 0;
        }
    }
    if (f.iyrt <= 0) {
        f.iftrgt = 0;
        f.begyrt = 0;
    } else {
        if (f.iftrgt == prm::NOTSET) f.iftrgt = 0;
        if (f.begyrt == prm::NOTSET) f.begyrt = 1;
        if (f.iyrt == 2) {
            if (f.lamda == prm::DNOTST) f.lamda = 0.0;
            if (f.rol == prm::DNOTST)
                f.rol = (sp == 12) ? 0.9 : std::pow(0.9, 12.0 / sp);
        }
    }
    ctx.captured.has_force = true;
}

// ---- slidingspans{} (getssp.f) --------------------------------------------
void gt_slidingspans(X13Context& ctx, bool& havesp, bool& inptok) {
    constexpr int PARG = 16;
    static const char ARGDIC[] =
        "startcutseascutchngcuttdoutlierfixmdlprintsavelengthtransparent"
        "savelogfixregadditivesanumspansx11outlierfixx11reg";
    static const int argptr[PARG + 1] = {1, 6, 13, 20, 25, 32, 38, 43, 47, 53,
        64, 71, 77, 87, 95, 105, 114};

    // Outlier-treatment dictionary (removekeepyes -> 0/1/2).
    static const char OTLDIC[] = "removekeepyes";
    static const int otlptr[4] = {1, 7, 11, 14};
    // fixmdl dictionary (noyesclear -> 0/1/2).
    static const char INTDIC[] = "noyesclear";
    static const int intptr[4] = {1, 3, 6, 11};
    // yes/no dictionary, shared by x11outlier/transparent/fixx11reg.
    static const char YSNDIC[] = "yesno";
    static const int ysnptr[3] = {1, 4, 6};
    // fixreg dictionary (tdholidayuseroutlier -> 1..4).
    static const char FXRDIC[] = "tdholidayuseroutlier";
    static const int fxrptr[5] = {1, 3, 10, 14, 21};
    // additivesa dictionary (differencepercent -> 1/2).
    static const char ADDDIC[] = "differencepercent";
    static const int addptr[3] = {1, 11, 18};

    // ssap.prm: MXCOL = max number of sliding spans, MXYR = max span length
    // in years.
    constexpr int MXCOL = 4;
    constexpr int MXYR = 23;

    // getssp.f entry defaults -- these are set once at the top of gtinpt.f
    // (lines 482-531), not inside getssp.f itself. Nothing reads ctx.sspinp /
    // ctx.hiddn.issap / ctx.captured.ssp_cut before slidingspans{} is parsed,
    // so setting them here is equivalent (same convention as gt_force above).
    sspinp_cmn& si = ctx.sspinp;
    ctx.hiddn.issap = 0;
    si.nlen = 0;
    si.ncol = 0;
    si.ssotl = 1;
    si.ssinit = 1;
    si.sstran = true;
    ctx.captured.ssp_cut = {3.0, 2.0, 3.0, 3.0, 3.0};
    si.strtss(1) = prm::NOTSET;
    si.strtss(2) = prm::NOTSET;
    si.ssfxrg(1) = prm::NOTSET;
    si.ssfxrg(2) = prm::NOTSET;
    si.ssfxrg(3) = prm::NOTSET;
    si.ssfxrg(4) = prm::NOTSET;
    si.nssfxr = 0;
    si.ssdiff = true;
    si.ssidif = true;
    si.ssxotl = true;
    si.ssxint = true;

    int arglog[2 * PARG];
    for (auto& v : arglog) v = -32767;
    int argidx;
    // getssp.f:77 -- argok=T, initialized once before the loop; each argument
    // reader below overwrites it as an out-param, and its value from the LAST
    // processed argument survives to the post-loop Inptok=Inptok.and.argok
    // check (getssp.f:262).
    bool argok = true;
    while (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
        if (ctx.error.lfatal) return;
        const int* ep = ctx.lex.errpos.data() + 1;
        int ivec[4] = {0, 0, 0, 0};
        double dvec[1] = {0.0};
        int nelt = 0;
        switch (argidx) {
        case 1:  // start
            gtdtvc(ctx, havesp, ctx.model.sp, LPAREN, false, 1,
                   si.strtss.data(), nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            break;
        case 2:  // cutseas
            gtdpvc(ctx, LPAREN, true, 1, dvec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) {
                if (dvec[0] <= 0.0) {
                    inpter(ctx, PERROR, ep,
                           "Value of cutseas must be greater than zero.");
                    inptok = false;
                } else {
                    ctx.captured.ssp_cut[0] = dvec[0];
                    ctx.captured.ssp_cut[2] = dvec[0];
                }
            }
            break;
        case 3:  // cutchng
            gtdpvc(ctx, LPAREN, true, 1, dvec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) {
                if (dvec[0] <= 0.0) {
                    inpter(ctx, PERROR, ep,
                           "Value of cutchng must be greater than zero.");
                    inptok = false;
                } else {
                    ctx.captured.ssp_cut[3] = dvec[0];
                    ctx.captured.ssp_cut[4] = dvec[0];
                }
            }
            break;
        case 4:  // cuttd
            gtdpvc(ctx, LPAREN, true, 1, dvec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) {
                if (dvec[0] <= 0.0) {
                    inpter(ctx, PERROR, ep,
                           "Value of cuttd must be greater than zero.");
                    inptok = false;
                } else {
                    ctx.captured.ssp_cut[1] = dvec[0];
                }
            }
            break;
        case 5:  // outlier: no|keep|yes (getssp.f's OTLDIC is remove/keep/yes,
                 // but "remove" is spelled with an r- prefix in the dictionary
                 // string; the argument keyword accepted is "outlier=remove|
                 // keep|yes").
            gtdcvc(ctx, LPAREN, true, 1, OTLDIC, otlptr, 3,
                   "Available options for outlier are no, keep, or yes.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) si.ssotl = ivec[0] - 1;
            break;
        case 6:  // fixmdl: no|clear|yes
            gtdcvc(ctx, LPAREN, true, 1, INTDIC, intptr, 3,
                   "Available options for fixmdl are no, clear, or yes.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) si.ssinit = ivec[0] - 1;
            break;
        case 7:  // print -- deferred table selection, consumed not stored.
            consume_value(ctx, nullptr);
            if (ctx.error.lfatal) return;
            break;
        case 8: {  // save
            std::vector<std::string> cap;
            consume_value(ctx, &cap);
            if (ctx.error.lfatal) return;
            for (const auto& t : cap) ctx.captured.save_tables.push_back(t);
            break;
        }
        case 9:  // length
            getivc(ctx, LPAREN, true, 1, ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (ivec[0] < 3 * ctx.model.sp) {
                inpter(ctx, PERROR, ep,
                       "Length of sliding spans must be greater than or "
                       "equal to 3 years.");
                inptok = false;
            } else if (ivec[0] > MXYR * ctx.model.sp) {
                inpter(ctx, PERROR, ep,
                       "Length of sliding spans must be less than or equal "
                       "to " + std::to_string(MXYR) + " years.");
                inptok = false;
            } else if (argok) {
                si.nlen = ivec[0];
            }
            break;
        case 10:  // transparent: no|yes
            gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                   "Available options for transparent are no or yes.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) si.sstran = (ivec[0] == 1);
            break;
        case 11:  // savelog -- deferred, consumed not stored.
            consume_value(ctx, nullptr);
            if (ctx.error.lfatal) return;
            break;
        case 12:  // fixreg: (td holiday user outlier)
            gtdcvc(ctx, LPAREN, true, 4, FXRDIC, fxrptr, 4,
                   "Available options for fixreg are td, holiday, or user.",
                   si.ssfxrg.data(), si.nssfxr, argok, inptok);
            if (ctx.error.lfatal) return;
            break;
        case 13:  // additivesa: difference|percent
            gtdcvc(ctx, LPAREN, true, 1, ADDDIC, addptr, 2,
                   "Available options for additivesa are difference or "
                   "percent.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) si.ssdiff = (ivec[0] == 1);
            break;
        case 14:  // numspans (a.k.a. "spans" in getssp.f's comments)
            getivc(ctx, LPAREN, true, 1, ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (ivec[0] <= 1) {
                inpter(ctx, PERROR, ep,
                       "Value of spans must be greater than one.");
                inptok = false;
            } else if (ivec[0] > MXCOL) {
                inpter(ctx, PERROR, ep,
                       "Value of spans must be less than or equal to " +
                       std::to_string(MXCOL) + ".");
                inptok = false;
            } else if (argok) {
                si.ncol = ivec[0];
            }
            break;
        case 15:  // x11outlier: no|yes
            gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                   "Available options for x11outlier are no or yes.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) si.ssxotl = (ivec[0] == 1);
            break;
        case 16:  // fixx11reg: no|yes
            gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                   "Available options for fixx11reg are no or yes.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) si.ssxint = (ivec[0] == 1);
            break;
        default:
            break;
        }
        if (ctx.error.lfatal) return;
    }

    // getssp.f:262-263 -- Issap=1 only if every argument parsed cleanly.
    inptok = inptok && argok;
    if (inptok) ctx.hiddn.issap = 1;
    ctx.captured.has_slidingspans = true;
}

// ---- history{} (gtrvst.f) --------------------------------------------------
// Revisions-history controls. Fills ctx.rev (rev_cmn) + ctx.hiddn.irev/irevsa,
// mirroring gtrvst.f. Entry defaults are the gtinpt.f:485-518 block (set once
// before the block is parsed; nothing reads ctx.rev earlier, same convention as
// gt_force/gt_slidingspans). The driver (revdrv) is scoped separately.
void gt_history(X13Context& ctx, bool& havesp, bool& inptok) {
    constexpr int PARG = 20;
    static const char ARGDIC[] =
        "estimatessadjlagstrendlagsfstepstartendtablefixmdltransparent"
        "refreshoutlieroutlierwintargetprintsavesavelogfixregx11outlier"
        "fixx11regadditivesatransformfcst";
    static const int argptr[PARG + 1] = {1, 10, 18, 27, 32, 37, 45, 51, 62, 69,
        76, 86, 92, 97, 101, 108, 114, 124, 133, 143, 156};

    // estimates dictionary (sadj/seasonal/sadjchng/aic/fcst/trend/trendchng/
    // arma/td -> 1..9).
    static const char ESTDIC[] = "sadjseasonalsadjchngaicfcsttrendtrendchngarmatd";
    static const int estptr[10] = {1, 5, 13, 21, 24, 28, 33, 42, 46, 48};
    static const char YSNDIC[] = "yesno";
    static const int ysnptr[3] = {1, 4, 6};
    static const char OTLDIC[] = "keepremoveauto";
    static const int otlptr[4] = {1, 5, 11, 15};
    static const char TRGDIC[] = "concurrentfinal";
    static const int trgptr[3] = {1, 11, 16};
    static const char FXRDIC[] = "tdholidayuseroutlier";
    static const int fxrptr[5] = {1, 3, 10, 14, 21};
    static const char ADDDIC[] = "differencepercent";
    static const int addptr[3] = {1, 11, 18};

    rev_cmn& rv = ctx.rev;
    revtrg_cmn& rt = ctx.revtrg;
    hiddn_cmn& hid = ctx.hiddn;
    const int sp = ctx.model.sp;

    // gtinpt.f:485-518 entry defaults.
    hid.irev = 0;
    hid.irevsa = 0;
    rv.cnctar = false;
    rt.ntarsa = 0;
    rt.ntartr = 0;
    rv.nfctlg = 0;
    rv.rvstrt(1) = 0;
    rv.rvstrt(2) = 0;
    rv.lrvsa = rv.lrvsf = rv.lrvch = rv.lrvtrn = rv.lrvtch = false;
    rv.lrvaic = rv.lrvfct = rv.lrvarma = rv.lrvtdrg = false;
    rv.revfix = false;
    rv.lrfrsh = false;
    rv.otlrev = 0;
    rv.otlwin = prm::NOTSET;
    rv.rvtran = true;
    rv.rvdiff = 2;
    rv.revfxx = false;
    rv.rvxotl = true;
    rv.rvtrfc = false;
    rv.nrvfxr = 0;

    int arglog[2 * PARG];
    for (auto& v : arglog) v = -32767;
    int argidx;
    bool argok = true;
    while (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
        if (ctx.error.lfatal) return;
        const int* ep = ctx.lex.errpos.data() + 1;
        int ivec[4] = {0, 0, 0, 0};
        int estidx[9] = {0};
        int nelt = 0;
        switch (argidx) {
        case 1: {  // estimates
            gtdcvc(ctx, LPAREN, false, 9, ESTDIC, estptr, 9,
                   "Choices of estimates are sadj, seasonal, sadjchng, trend, "
                   "trendchng, aic, fcst, arma, and td.",
                   estidx, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok) {
                for (int i = 0; i < nelt; ++i) {
                    switch (estidx[i]) {
                    case 1: rv.lrvsa = true; break;
                    case 2: rv.lrvsf = true; break;
                    case 3: rv.lrvch = true; break;
                    case 4: rv.lrvaic = true; break;
                    case 5: rv.lrvfct = true; break;
                    case 6: rv.lrvtrn = true; break;
                    case 7: rv.lrvtch = true; break;
                    case 8: rv.lrvarma = true; break;
                    case 9: rv.lrvtdrg = true; break;
                    }
                }
            }
            break;
        }
        case 2:  // sadjlags
            getivc(ctx, LPAREN, true, 5, rt.targsa.data(), rt.ntarsa,
                   argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && rt.ntarsa > 0)
                for (int i = 1; i <= rt.ntarsa; ++i)
                    if (rt.targsa(i) <= 0) {
                        inpter(ctx, PERROR, ep,
                               "Entries for sadjlags must be greater than zero.");
                        inptok = false;
                    }
            break;
        case 3:  // trendlags
            getivc(ctx, LPAREN, true, 5, rt.targtr.data(), rt.ntartr,
                   argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && rt.ntartr > 0)
                for (int i = 1; i <= rt.ntartr; ++i)
                    if (rt.targtr(i) <= 0) {
                        inpter(ctx, PERROR, ep,
                               "Entries for trendlags must be greater than zero.");
                        inptok = false;
                    }
            break;
        case 4:  // fstep
            getivc(ctx, LPAREN, true, 4, rv.rfctlg.data(), rv.nfctlg,
                   argok, inptok);
            if (ctx.error.lfatal) return;
            break;
        case 5:  // start
            gtdtvc(ctx, havesp, ctx.model.sp, LPAREN, false, 1,
                   rv.rvstrt.data(), nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            break;
        case 6:  // endtable
            gtdtvc(ctx, havesp, ctx.model.sp, LPAREN, false, 1,
                   rv.rvend.data(), nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            break;
        case 7:  // fixmdl
            gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                   "Available options for fixmdl are no or yes.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) rv.revfix = (ivec[0] == 1);
            break;
        case 8:  // transparent
            gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                   "Available options for transparent are no or yes.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) rv.rvtran = (ivec[0] == 1);
            break;
        case 9:  // refresh
            gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                   "Available options for refresh are no or yes.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) rv.lrfrsh = (ivec[0] == 1);
            break;
        case 10: {  // outlier
            int ivec2[2] = {0, 0};
            gtdcvc(ctx, LPAREN, true, 2, OTLDIC, otlptr, 3,
                   "Available options for outlier are remove, keep or auto.",
                   ivec2, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok) {
                rv.otlrev = 0;
                for (int i = 0; i < nelt; ++i) rv.otlrev += (ivec2[i] - 1);
                if (nelt == 2 && rv.otlrev == 1) {
                    inpter(ctx, PERROR, ep, "Cannot specify both remove and "
                           "keep for the outlier argument.");
                    inptok = false;
                }
            }
            break;
        }
        case 11:  // outlierwin
            getivc(ctx, LPAREN, true, 1, ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            rv.otlwin = ivec[0];
            if (argok && rv.otlwin < 0) {
                inpter(ctx, PERROR, ep, "Value of outlierwin must be an integer "
                       "greater than or equal to zero.");
                inptok = false;
            }
            break;
        case 12:  // target
            gtdcvc(ctx, LPAREN, true, 1, TRGDIC, trgptr, 2,
                   "Available options for target are concurrent or final.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) rv.cnctar = (ivec[0] == 1);
            break;
        case 13:  // print
            consume_value(ctx, nullptr);
            break;
        case 14:  // save
            consume_value(ctx, nullptr);
            break;
        case 15:  // savelog
            consume_value(ctx, nullptr);
            break;
        case 16:  // fixreg
            gtdcvc(ctx, LPAREN, true, 4, FXRDIC, fxrptr, 4,
                   "Available options for fixreg are td, holiday, or user.",
                   rv.rvfxrg.data(), rv.nrvfxr, argok, inptok);
            if (ctx.error.lfatal) return;
            break;
        case 17:  // x11outlier
            gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                   "Available options for x11outlier are no or yes.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) rv.rvxotl = (ivec[0] == 1);
            break;
        case 18:  // fixx11reg
            gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                   "Available options for fixx11reg are no or yes.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) rv.revfxx = (ivec[0] == 1);
            break;
        case 19:  // additivesa
            gtdcvc(ctx, LPAREN, true, 1, ADDDIC, addptr, 2,
                   "Available options for additivesa are difference or percent.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) rv.rvdiff = ivec[0];
            break;
        case 20:  // transformfcst
            gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                   "Available options for transformfcst are no or yes.",
                   ivec, nelt, argok, inptok);
            if (ctx.error.lfatal) return;
            if (argok && nelt > 0) rv.rvtrfc = (ivec[0] == 1);
            break;
        }
        if (ctx.error.lfatal) return;
    }
    if (ctx.error.lfatal) return;

    // Post-parse checks (gtrvst.f:349-360).
    if (hid.irev == 0) hid.irev = 1;
    if (!rv.lrvsa && rt.ntarsa > 0) rv.lrvsa = true;
    if (!rv.lrvtrn && rt.ntartr > 0) rv.lrvtrn = true;
    if (!rv.lrvsa && !rv.lrvsf && !rv.lrvch && !rv.lrvtrn && !rv.lrvaic &&
        !rv.lrvfct && !rv.lrvtch && !rv.lrvarma && !rv.lrvtdrg)
        rv.lrvsa = true;
    if (rv.lrvsa || rv.lrvsf || rv.lrvch || rv.lrvtrn || rv.lrvtch)
        hid.irevsa = 1;
    if (rv.otlwin == prm::NOTSET) rv.otlwin = sp;

    // gtrvst.f:361-435 -- composite{} indirect-revision bookkeeping. Indrev and
    // Indrvs are /revcmn/ COMMONs that agr1 initializes once for the whole
    // metafile (NOTSET / 0.0) and that every component spec then narrows; the
    // metafile driver carries them between specs. Indrev defaults to "on"
    // whenever the FIRST component asks for a seasonal-adjustment history, and
    // is switched off the moment any component disagrees -- it does not ask for
    // a sadj history, or its history start date differs, or it has none at all.
    if (ctx.agr.iagr > 0) {
        constexpr int YR = 1, MO = 2;
        rev_cmn& ir = ctx.rev;
        bool lprt2 = false;
        if (ir.indrev == prm::NOTSET) ir.indrev = rv.lrvsa ? 1 : 0;
        if (ir.indrev == 1) {
            if (!rv.lrvsa) {
                ir.indrev = 0;
                writln(ctx, "WARNING: Need to specify revisons history for "
                            "seasonal adjustments in all components of a "
                            "composite adjustment to get a revisions history "
                            "of the indirect seasonally adjusted series.",
                       stdio::STDERR, ctx.units.mt2, true);
                lprt2 = true;
            } else if (rv.rvstrt(YR) > 0) {
                if (ir.indrvs(YR) == 0) {
                    ir.indrvs(YR) = rv.rvstrt(YR);
                    ir.indrvs(MO) = rv.rvstrt(MO);
                } else if (!(ir.indrvs(YR) == rv.rvstrt(YR) &&
                             ir.indrvs(MO) == rv.rvstrt(MO))) {
                    ir.indrev = 0;
                    writln(ctx, "WARNING: Starting date of revisons history "
                                "analysis must be the same for all components "
                                "of a composite adjustment to get a revisions "
                                "history of the indirect seasonally adjusted "
                                "series.",
                           stdio::STDERR, ctx.units.mt2, true);
                    lprt2 = true;
                }
            } else if (rv.rvstrt(YR) == 0 && ir.indrev > 0) {
                ir.indrev = 0;
                writln(ctx, "WARNING: Starting date of revisons history "
                            "analysis must be specified for all components of "
                            "a composite adjustment to get a revisions history "
                            "of the indirect seasonally adjusted series.",
                       stdio::STDERR, ctx.units.mt2, true);
                lprt2 = true;
            }
        }
        if (lprt2)
            writln(ctx, "         Edit all input specification files to "
                        "correct this and rerun the metafile.",
                   stdio::STDERR, ctx.units.mt2, false);
    }

    inptok = inptok && argok;
    ctx.captured.has_history = true;
}

// ---- check{} (getchk.f) ----------------------------------------------------
void gt_check(X13Context& ctx, bool& inptok) {
    constexpr int PARG = 7;
    static const char ARGDIC[] = "maxlagprintsavesavelogacflimitqtypeqlimit";
    static const int argptr[PARG + 1] = {1, 7, 12, 16, 23, 31, 36, 42};
    gt_generic(ctx, ARGDIC, argptr, PARG, inptok);
}

// ---- identify{} (getid.f) --------------------------------------------------
void gt_identify(X13Context& ctx, bool& inptok) {
    constexpr int PARG = 5;
    static const char ARGDIC[] = "diffsdiffmaxlagprintsave";
    static const int argptr[PARG + 1] = {1, 5, 10, 16, 21, 25};
    gt_generic(ctx, ARGDIC, argptr, PARG, inptok);
}

// ---- composite{} (getcmp.f) ----------------------------------------------
// The composite spec REPLACES series{} in the last spec of a metafile run: its
// data is the aggregate the component runs accumulated into `O`. So this reader
// parses its options, derives the span from Itest(1..5) (stamped by the first
// component -- see composite/agr2.cpp), and then agr1() hands `O` over as the
// series. `lagr` is getcmp's Locok out-parameter (gtinpt.f:790 uses it to set
// havsrs), NOT the series{} comptype flag.
void gt_composite(X13Context& ctx, bool& havsrs, bool& havesp, bool& lagr,
                  bool& inptok) {
    LexState& L = ctx.lex;
    constexpr int PARG = 13;
    static const char ARGDIC[] =
        "nametitleprintsavedecimalsmodelspansaveprecisionsavelogyr2000"
        "indoutlierappendfcstappendbcsttype";
    static const int argptr[PARG + 1] = {1, 5, 10, 15, 19, 27, 36, 49, 56, 62, 72,
        82, 92, 96};
    static const char YSNDIC[] = "yesno";
    static const int ysnptr[3] = {1, 4, 6};
    static const char TYPDIC[] = "flowstock";
    static const int typptr[3] = {1, 5, 10};

    constexpr int YR = 1, MO = 2;   // 1-based date components (Fortran order)
    int& sp = ctx.model.sp;
    double* y = ctx.arima.y.data();
    int& nobs = ctx.arima.nobs;
    int* start = ctx.arima.begsrs.data();
    int* begspn = ctx.mdldat.begspn.data();
    int& nspobs = ctx.mdldat.nspobs;
    int* begmdl = ctx.arima.begmdl.data();
    int* endmdl = ctx.arima.endmdl.data();

    bool locok = true;
    int spnmdl[4], endspn[2];
    setint(prm::NOTSET, 4, spnmdl);
    setint(prm::NOTSET, 2, endspn);
    int arglog[2 * PARG];
    setint(prm::NOTSET, 2 * PARG, arglog);

    std::string srsttl(prm::PSRSCR, ' ');
    std::string srsnam(64, ' ');
    int nttlcr = 0, nser = 0;
    int tmpptr[2];
    int ivec[1];
    int nelt = 0;
    bool argok = false;
    bool hvnam = false;

    int argidx;
    while (true) {
        if (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
            if (ctx.error.lfatal) return;
            switch (argidx) {
            case 1:  // name (getcmp.f:79-83)
                gtnmvc(ctx, LPAREN, true, 1, srsnam, tmpptr, nelt, 64, argok, locok);
                if (ctx.error.lfatal) return;
                if (argok) { eltlen(ctx, 1, tmpptr, nelt, nser); hvnam = true; }
                break;
            case 2:  // title (getcmp.f:87-91)
                getttl(ctx, LPAREN, true, 1, srsttl, tmpptr, nelt, argok, locok);
                if (!ctx.error.lfatal && argok && nelt == 1)
                    eltlen(ctx, 1, tmpptr, nelt, nttlcr);
                if (ctx.error.lfatal) return;
                break;
            case 3:  // print
                getprt(ctx, 0, 10, locok);
                break;
            case 4:  // save
                getsav(ctx, 0, 10, locok);
                break;
            case 5:  // decimals (getcmp.f:104-116)
                getivc(ctx, LPAREN, true, 1, ivec, nelt, argok, locok);
                if (ctx.error.lfatal) return;
                if (argok) {
                    if (ivec[0] < 0 || ivec[0] > 5) {
                        inpter(ctx, PERROR, L.errpos.data() + 1,
                               "Number of output decimals must be between 0 and 5, inclusive.");
                        locok = false;
                    } else {
                        ctx.x11opt.kdec = ivec[0];
                    }
                }
                break;
            case 6:  // modelspan (getcmp.f:120-128)
                gtdtvc(ctx, havesp, sp, LPAREN, false, 2, spnmdl, nelt, argok, inptok);
                if (ctx.error.lfatal) return;
                if (nelt == 1) {
                    inpter(ctx, PERROR, L.errpos.data() + 1,
                           "Need two dates for the model span or use a comma as place holder.");
                    inptok = false;
                }
                break;
            case 7:  // saveprecision (getcmp.f:132-144)
                getivc(ctx, LPAREN, true, 1, ivec, nelt, argok, locok);
                if (ctx.error.lfatal) return;
                if (argok && nelt > 0) {
                    if (ivec[0] <= 0 || ivec[0] > 15) {
                        inpter(ctx, PERROR, L.errpos.data() + 1,
                               "Value of saveprecision must be greater than zero and less than 15.");
                        inptok = false;
                    } else {
                        ctx.savcmn.svprec = ivec[0];
                    }
                }
                break;
            case 8:  // savelog
                getsvl(ctx, 0, 10, locok);
                break;
            case 9:  // yr2000 (getcmp.f:151-158)
                gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                       "Available options for yr2000 are yes or no.", ivec, nelt,
                       argok, inptok);
                if (ctx.error.lfatal) return;
                if (argok && nelt > 0) ctx.x11msc.yr2000 = (ivec[0] == 1);
                break;
            case 10:  // indoutlier (getcmp.f:162-169)
                gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                       "Available options for indoutlier are yes or no.", ivec,
                       nelt, argok, inptok);
                if (ctx.error.lfatal) return;
                if (argok && nelt > 0) ctx.agr.lindot = (ivec[0] == 1);
                break;
            case 11:  // appendfcst (getcmp.f:173-180) -- same Savfct as x11{}
                gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                       "Available options for appending forecasts are yes or no.",
                       ivec, nelt, argok, inptok);
                if (ctx.error.lfatal) return;
                if (argok && nelt > 0) ctx.tbllog.savfct = (ivec[0] == 1);
                break;
            case 12:  // appendbcst (getcmp.f:184-191)
                gtdcvc(ctx, LPAREN, true, 1, YSNDIC, ysnptr, 2,
                       "Available options for appending backcasts are yes or no.",
                       ivec, nelt, argok, inptok);
                if (ctx.error.lfatal) return;
                if (argok && nelt > 0) ctx.tbllog.savbct = (ivec[0] == 1);
                break;
            case 13:  // type (getcmp.f:195-201)
                gtdcvc(ctx, LPAREN, true, 1, TYPDIC, typptr, 2,
                       "Available options for type are flow or stock.", ivec,
                       nelt, argok, inptok);
                if (ctx.error.lfatal) return;
                if (argok && nelt > 0) ctx.model.isrflw = ivec[0];
                break;
            default:
                break;
            }
            if (ctx.error.lfatal) return;
            continue;   // GO TO 140
        }
        if (ctx.error.lfatal) return;

        // --- End of argument loop (getcmp.f:205-247) ---
        // The span comes from Itest(1..5) = (Sp, BegMo, EndMo, BegYr, EndYr),
        // stamped by the first component series.
        start[YR - 1] = ctx.agr.itest(4);
        start[MO - 1] = ctx.agr.itest(2);
        begspn[YR - 1] = ctx.agr.itest(4);
        begspn[MO - 1] = ctx.agr.itest(2);
        endspn[YR - 1] = ctx.agr.itest(5);
        endspn[MO - 1] = ctx.agr.itest(3);
        sp = ctx.agr.itest(1);
        // getcmp.f:213 Havesp=T -- the composite spec supplies the seasonal
        // period (from Itest) for every LATER spec in the file. Without this a
        // composite total carrying x11{seasonalma=} fatals with "No seasonal
        // period specified in series spec": there is no series{} to set it.
        havesp = true;
        dfdate(endspn, begspn, sp, nspobs);
        nspobs = nspobs + 1;
        nobs = nspobs;

        if (spnmdl[YR - 1] == prm::NOTSET) {
            cpyint(begspn, 2, 1, begmdl);
        } else {
            cpyint(spnmdl, 2, 1, begmdl);
        }
        if (spnmdl[2 + YR - 1] == prm::NOTSET || spnmdl[2 + YR - 1] == 0) {
            addate(begspn, sp, nspobs - 1, endmdl);
            if (spnmdl[2 + YR - 1] == 0) {
                endmdl[MO - 1] = spnmdl[2 + MO - 1];
                if (endmdl[MO - 1] > endspn[MO - 1]) endmdl[YR - 1] -= 1;
            }
        } else {
            cpyint(spnmdl + 2, 2, 1, endmdl);
        }
        int nobmdl = 0;
        dfdate(endmdl, begmdl, sp, nobmdl);
        nobmdl = nobmdl + 1;
        if (!chkcvr(begspn, nspobs, begmdl, nobmdl, sp)) {
            inpter(ctx, PERRNP, L.errpos.data() + 1,
                   "Model span is not within the span of available data.");
            if (ctx.error.lfatal) return;
            inptok = false;
        }
        // getcmp.f:241 -- hand the accumulated composite total over as the series.
        if (locok) {
            agr1(ctx, y, nobs);
            havsrs = true;
        }
        if (ctx.model.isrflw == prm::NOTSET) ctx.model.isrflw = 0;

        // --- capture for the gate (mirrors getsrs) ---
        if (havsrs) {
            ctx.captured.has_series = true;
            ctx.captured.period = sp;
            ctx.captured.nobs = nobs;
            ctx.captured.series_start = {start[0], start[1]};
            ctx.captured.span_start = {begspn[0], begspn[1]};
            ctx.captured.span_end = {endspn[0], endspn[1]};
            std::string t = srsttl.substr(0, static_cast<std::size_t>(nttlcr > 0 ? nttlcr : 0));
            if (!t.empty()) ctx.captured.title = t;
            if (hvnam) {
                std::string nm = srsnam.substr(0, static_cast<std::size_t>(nser > 0 ? nser : 0));
                if (!nm.empty()) ctx.captured.series_name = nm;
            }
        }
        lagr = locok;              // getcmp's Locok out-parameter
        inptok = inptok && locok;
        return;
    }
}

// ---- metadata{} (gtmtdt.f) ------------------------------------------------
// User-defined descriptive key/value pairs echoed onto the diagnostic (.udg)
// surface. Parse-only for the port: the resolved pairs are captured onto
// ctx.metadata (the /cmtdat//cmtdic/ common block); no auto file output.
void gt_metadata(X13Context& ctx, bool& inptok) {
    LexState& L = ctx.lex;

    // metadata argument data dictionary (gtmtdt.f:22-31).
    constexpr int PMETA = 2;
    static const char MDTDIC[] = "keysvalues";
    static const int mdtptr[PMETA + 1] = {1, 5, 11};

    // metadata.prm: PMTDAT = max number of metadata values, PLMETA = buffer len.
    constexpr int PMTDAT = 20;
    constexpr int PLMETA = 2000;

    // Local mirror of the /cmtdat//cmtdic/ common block, initialized as in
    // gtinpt.f:553-557. getttl grows these std::string buffers (pre-sized to
    // PLMETA like the Fortran CHARACTER*(PLMETA)); persisted to ctx.metadata at
    // the end. Element i (1-based) occupies buf[ptr[i-1] .. ptr[i]-1] (1-based).
    std::string keystr(PLMETA, ' '), valstr(PLMETA, ' ');
    int keyptr[PMTDAT + 1], valptr[PMTDAT + 1];
    int nkey = 0, nval = 0;
    bool hvmtdt = false, argok = true;
    intlst(PMTDAT, keyptr, nkey);
    intlst(PMTDAT, valptr, nval);

    // Extract element i (1-based) into out; returns its length (getstr.f).
    auto get_elt = [](const std::string& buf, const int* ptr, int i,
                      std::string& out) -> int {
        int nchr = ptr[i] - ptr[i - 1];
        out = nchr > 0 ? buf.substr(static_cast<std::size_t>(ptr[i - 1] - 1),
                                    static_cast<std::size_t>(nchr))
                       : std::string();
        return nchr;
    };
    // Append a new element to the (buf, ptr, n) list (insstr.f at the end == an
    // append; the synthesis paths below only ever append).
    auto append_elt = [](std::string& buf, int* ptr, int& n,
                         const std::string& s) {
        int start = ptr[n];   // 1-based next free position
        for (std::size_t k = 0; k < s.size(); ++k)
            buf[static_cast<std::size_t>(start - 1) + k] = s[k];
        ptr[n + 1] = start + static_cast<int>(s.size());
        n = n + 1;
    };

    int mdtlog[2 * PMETA];
    for (auto& v : mdtlog) v = -32767;   // NOTSET

    // --- Argument get loop (gtmtdt.f:38-58) ---
    int mdtidx;
    while (gtarg(ctx, MDTDIC, mdtptr, PMETA, mdtidx, mdtlog, inptok)) {
        if (ctx.error.lfatal) return;
        switch (mdtidx) {
        case 1:  // keys
            getttl(ctx, LPAREN, true, PMTDAT, keystr, keyptr, nkey, argok, inptok);
            if (ctx.error.lfatal) return;
            break;
        case 2:  // values
            getttl(ctx, LPAREN, true, PMTDAT, valstr, valptr, nval, argok, inptok);
            if (ctx.error.lfatal) return;
            break;
        }
    }
    if (ctx.error.lfatal) return;
    if (argok) hvmtdt = true;

    const int* pos = L.pos.data() + 1;   // gtmtdt.f uses Pos for inpter

    // --- Check keys for prohibited characters (gtmtdt.f:64-87) ---
    if (nkey > 0) {
        for (int i = 1; i <= nkey; ++i) {
            if (!argok) break;
            std::string thisky;
            get_elt(keystr, keyptr, i, thisky);
            if (thisky.find(' ') != std::string::npos) {
                inpter(ctx, PERRNP, pos,
                       "Keys specified in metadata spec cannot contain spaces.");
                hvmtdt = false;
                argok = false;
            }
            if (argok && thisky.find(':') != std::string::npos) {
                inpter(ctx, PERRNP, pos,
                       "Keys specified in metadata spec cannot contain colons.");
                hvmtdt = false;
                argok = false;
            }
        }
    }

    // --- Key/value count reconciliation (gtmtdt.f:92-141) ---
    if (nkey == 0 && nval > 0) {
        // No keys given: synthesize 'keyN' for each value.
        keyptr[0] = 1;
        for (int i = 1; i <= nval; ++i) {
            std::string ikeystr(10, ' ');
            ikeystr[0] = 'k'; ikeystr[1] = 'e'; ikeystr[2] = 'y';
            int ikey = 4;
            itoc(ctx, i, ikeystr, ikey);
            if (ctx.error.lfatal) return;
            append_elt(keystr, keyptr, nkey,
                       ikeystr.substr(0, static_cast<std::size_t>(ikey - 1)));
        }
    } else if (nval > nkey) {
        // Fewer keys than values: warn, then synthesize the missing 'keyN'.
        std::string ckey(5, ' '); int ikey = 1; itoc(ctx, nkey, ckey, ikey);
        if (ctx.error.lfatal) return;
        std::string cval(5, ' '); int ival = 1; itoc(ctx, nval, cval, ival);
        if (ctx.error.lfatal) return;
        inpter(ctx, PWRNNP, pos,
               "Fewer keys (" + ckey.substr(0, static_cast<std::size_t>(ikey - 1)) +
               ") than values (" + cval.substr(0, static_cast<std::size_t>(ival - 1)) +
               ") specified in metadata spec.");
        for (int i = nkey + 1; i <= nval; ++i) {
            std::string thisky(10, ' ');
            thisky[0] = 'k'; thisky[1] = 'e'; thisky[2] = 'y';
            int ik = 4;
            itoc(ctx, i, thisky, ik);
            if (ctx.error.lfatal) return;
            append_elt(keystr, keyptr, nkey,
                       thisky.substr(0, static_cast<std::size_t>(ik - 1)));
        }
    } else if (nval < nkey) {
        // Fewer values than keys: error. NOTE the message text swaps the two
        // counts (ckey holds Nkey but is printed as "values", cval holds Nval
        // but is printed as "keys") -- ported verbatim from gtmtdt.f:132-134
        // (Census bug CB, reproduced exactly).
        std::string ckey(5, ' '); int ikey = 1; itoc(ctx, nkey, ckey, ikey);
        if (ctx.error.lfatal) return;
        std::string cval(5, ' '); int ival = 1; itoc(ctx, nval, cval, ival);
        if (ctx.error.lfatal) return;
        inpter(ctx, PERRNP, pos,
               "Fewer values (" + ckey.substr(0, static_cast<std::size_t>(ikey - 1)) +
               ") than keys (" + cval.substr(0, static_cast<std::size_t>(ival - 1)) +
               ") specified in metadata spec.");
        hvmtdt = false;
        argok = false;
    } else if (nval == 0 && nkey == 0) {
        hvmtdt = false;
    }

    // --- Check that key values are unique (gtmtdt.f:145-161) ---
    if (argok && hvmtdt) {
        for (int i = 1; i <= nval - 1; ++i) {
            std::string thisky;
            int ikey = get_elt(keystr, keyptr, i, thisky);
            for (int j = i + 1; j <= nval; ++j) {
                std::string thatky;
                int jkey = get_elt(keystr, keyptr, j, thatky);
                if (ikey == jkey && thisky == thatky) {
                    inpter(ctx, PERRNP, pos, "Key values must be unique.");
                    hvmtdt = false;
                    argok = false;
                }
            }
        }
    }

    // Persist the resolved metadata onto ctx (the result object). No file output.
    metadata_cmn& m = ctx.metadata;
    m.keystr = keystr;
    m.valstr = valstr;
    for (int i = 0; i <= PMTDAT; ++i) { m.keyptr(i) = keyptr[i]; m.valptr(i) = valptr[i]; }
    m.nkey = nkey;
    m.nval = nval;
    m.hvmtdt = hvmtdt;

    inptok = inptok && argok;
}

// ---- spectrum{} (gtspec.f) ------------------------------------------------
// Spectral-diagnostic block. Parse-acceptance only for now: the ARGDIC/argptr
// are copied verbatim from gtspec.f so the block's args are recognized and
// consumed; per-arg validation and the spectrum-table compute (spgrh/spgrh2/
// gendff/spcrsd/Tukey via spcdrv.f) are follow-on work.
void gt_spectrum(X13Context& ctx, bool& inptok) {
    constexpr int PARG = 21;
    static const char ARGDIC[] =
        "startdifferencetypeseriessiglevelpeakwidthmaxara"
        "ltfreqaxisprintsavesavelogsaveallfreqdecibellocalpeakstartdiffshow"
        "seasonalfreqtukey120logqsqcheckrobustsa";
    static const int argptr[PARG + 1] = {1, 6, 16, 20, 26, 34, 43, 48, 55, 59, 64,
        68, 75, 86, 93, 102, 111, 127, 135, 140, 146, 154};

    // gtinpt.f:354-379 defaults (set for every run; captured here onto /rho/ so
    // run_spectrum can read them). NOTE the default type is arspec (Spctyp=0);
    // periodogram is an explicit override.
    auto& r = ctx.rho;
    r.spcdff = true;
    r.spdfor = prm::NOTSET;
    r.lstdff = false;
    r.svallf = false;
    r.ldecbl = true;
    r.spctyp = 0;
    r.spcsrs = 2;
    r.mxarsp = prm::NOTSET;
    r.spclim = 6.0;
    r.peakwd = prm::NOTSET;
    r.plocal = 0.002;
    r.bgspec(1) = prm::NOTSET;
    r.bgspec(2) = prm::NOTSET;
    r.axsame = false;
    ctx.spcout.requested = true;

    int arglog[2 * PARG];
    for (auto& v : arglog) v = -32767;   // NOTSET
    int argidx;
    while (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
        if (ctx.error.lfatal) return;
        // Capture the value tokens for the options run_spectrum needs; the rest
        // are consumed token-faithfully without application. Deferred to later
        // increments (parsed + consumed here, not yet applied): start (arg 1 ->
        // Bgspec override; run_spectrum currently always uses the gtspec.f:324
        // eight-years-back default), peakwidth/altfreq/showseasonalfreq (arg
        // 6/8/17 -> the mkfreq peak-frequency grid), and the peaks/plots/qcheck/
        // Tukey/arspec compute.
        std::vector<std::string> cap;
        const bool want = (argidx == 2 || argidx == 3 || argidx == 4 ||
                           argidx == 7 || argidx == 14 || argidx == 16);
        consume_value(ctx, want ? &cap : nullptr);
        if (ctx.error.lfatal) return;
        if (cap.empty()) continue;
        const std::string& v = cap[0];
        switch (argidx) {
        case 2:  // difference: yes | first | no (gtspec.f:117-124)
            r.spcdff = (v != "no");
            if (v == "first") r.spdfor = 1;
            else if (v == "no") r.spdfor = 0;
            break;
        case 3:  // type: arspec | periodogram (gtspec.f:133)
            r.spctyp = (v == "periodogram") ? 1 : 0;
            break;
        case 7:  // maxar: AR-spectrum order, 1..30 (gtspec.f:157-164)
            try {
                const int mx = std::stoi(v);
                if (mx >= 1 && mx <= 30) r.mxarsp = mx;
            } catch (...) { /* range/format error handled by the Fortran path */ }
            break;
        case 4:  // series (gtspec.f:148-149): Spcsrs=ivec-1; if >3 subtract 4
            if (v == "original")              r.spcsrs = 0;
            else if (v == "outlieradjoriginal") r.spcsrs = 1;
            else if (v == "adjoriginal")      r.spcsrs = 2;
            else if (v == "modoriginal")      r.spcsrs = 3;
            else if (v == "a1")               r.spcsrs = 0;
            else if (v == "a19")              r.spcsrs = 1;
            else if (v == "b1")               r.spcsrs = 2;
            else if (v == "e1")               r.spcsrs = 3;
            break;
        case 14:  // decibel: yes | no (gtspec.f:255)
            r.ldecbl = (v == "yes");
            break;
        case 16:  // startdiff: yes | no (gtspec.f:279)
            r.lstdff = (v == "yes");
            break;
        default:
            break;
        }
    }
}

// ---- pickmdl{} (gtautx.f) -------------------------------------------------
// Classic X-11-ARIMA candidate-model selection. Parse-acceptance only; the
// .mdl candidate-file read + the estimate/forecast-error selection loop are
// follow-on.
void gt_pickmdl(X13Context& ctx, bool& inptok) {
    constexpr int PARG = 11;
    static const char ARGDIC[] =
        "modefileqlimfcstlimbcstlimoverdiffprintmethodout"
        "ofsampleidentifysavelog";
    static const int argptr[PARG + 1] = {1, 5, 9, 13, 20, 27, 35, 40, 46, 57, 65, 72};
    gt_generic(ctx, ARGDIC, argptr, PARG, inptok);
}

// ---- x11regression{} (gtxreg.f) -------------------------------------------
// Irregular-component regression. Builds the TD design via gtpdrg (x11reg=true)
// and sets Ixreg=1 / Axrgtd so x11pt2's B/C iterations run x11mdl_td. TD-only
// path; holiday/user/aictest/prior/outlier args are token-consumed (deferred).
void gt_x11regression(X13Context& ctx, bool havsrs, bool havesp, bool& inptok) {
    constexpr int PARG = 36;
    static const char ARGDIC[] =
        "variablesuserdatastartfileformatbprintsaveuserty"
        "pesigmacriticalumdataumstartumfileumformatumnameoutliermethodaicte"
        "sttdpriornoapplyholidaynonlineastermeansforcecalspanoutlierspanump"
        "recisionaicdiffsavelogumtrimzerocenteruserreweightcriticalalphadef"
        "aultcriticalprioralmost";
    static const int argptr[PARG + 1] = {1, 10, 14, 18, 23, 27, 33, 34, 39, 43, 51,
        56, 64, 70, 77, 83, 91, 97, 110, 117, 124, 131, 144, 155, 163, 167, 178,
        189, 196, 203, 213, 223, 231, 244, 259, 264, 272};
    LexState& L = ctx.lex;
    bool havtd = false, havhol = false, havln = false, havlp = false;
    int arglog[2 * PARG];
    for (auto& v : arglog) v = -32767;
    // gtinpt.f:804-816 ssprep+dlrgef: clear the regARIMA regressors so gtxreg
    // parses the x11reg variables into a bare model. On the bare-ARIMA corpus
    // path the model has no regressors, so the clear is exact; a regARIMA model
    // that already carries regressors needs the full ssprep/restor snapshot
    // (deferred -- xrg_clear_working leaves that case as future work).
    xrg_clear_working(ctx);
    int argidx;
    while (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
        if (ctx.error.lfatal) return;
        if (argidx == 1) {           // variables -> gtpdrg (x11reg=true)
            if (L.nxtktp == lexprm::EQUALS) lex(ctx);
            bool locok = true;
            gtpdrg(ctx, ctx.arima.begsrs.data(), ctx.arima.endmdl.data(),
                   ctx.arima.nobs, havsrs, havesp, /*x11reg=*/true, havtd, havhol,
                   havln, havlp, locok, inptok);
            if (ctx.error.lfatal) return;
        } else if (argidx == 20) {   // tdprior -> Dwt (gtxreg.f:416-423)
            if (L.nxtktp == lexprm::EQUALS) lex(ctx);
            int neltdw = 0;
            bool argok = true;
            gtdpvc(ctx, LPAREN, true, 7, ctx.x11reg.dwt.data(), neltdw, argok,
                   inptok);
            if (ctx.error.lfatal) return;
            if (argok && neltdw != 7) {
                inpter(ctx, PERROR, ctx.lex.errpos.data() + 1,
                       "Must have seven prior trading day weights.");
                inptok = false;
            }
        } else if (argidx == 19) {   // aictest (gtxreg.f:387-411)
            std::vector<std::string> toks;
            consume_value(ctx, &toks);
            if (ctx.error.lfatal) return;
            for (const auto& t0 : toks) {
                std::string s;
                for (char c : t0)
                    s += static_cast<char>(
                        std::tolower(static_cast<unsigned char>(c)));
                if (s == "easter") {   // gtxreg.f:395-397: Xeastr=T, Havxhl=T
                    ctx.x11log.xeastr = true;
                    // editor.f:1577-1590: no explicit Easter group in the model
                    // -> the default window set {0,1,8,15} to AIC-test over.
                    ctx.x11reg.xeasvc(1) = 0;
                    ctx.x11reg.xeasvc(2) = 1;
                    ctx.x11reg.xeasvc(3) = 8;
                    ctx.x11reg.xeasvc(4) = 15;
                    ctx.x11reg.neasvx = 4;
                }
                // td / tdstock / user aictest tokens: deferred (follow-on
                // increment; this gate is aictest=(easter)).
            }
        } else {
            consume_value(ctx, nullptr);
            if (ctx.error.lfatal) return;
        }
    }
    // gtinpt.f:830 loadxr(T): move the parsed x11reg model into ctx.xrgmdl so
    // the regARIMA estimate stays the bare ARIMA model (the oracle fits np=3, no
    // TD -- the x11reg estimates are not ML). gtinpt.f:832 restor: restore the
    // regARIMA model (bare ARIMA -> clear the working regressors back out).
    // gtxreg.f:186-192: for the multiplicative/log-additive td regression the
    // length-of-month / leap-year variation is carried by the Xnstar day-count
    // normalization (x11ref), not a regression column -- rmlnvr strips the Leap
    // Year regressor that gtpdrg's picktd "td" adds, so the x11reg model is the
    // 6 day-contrasts alone (the oracle's nxreg=6). The prior-adjust arg is a
    // throwaway here (NOTSET): x11reg does not fold the LOM into a series prior.
    if (ctx.picktd.picktd && ctx.prior.priadj != prm::NOTSET &&
        ctx.prior.priadj != 1) {
        int tmppa = prm::NOTSET;
        rmlnvr(ctx, tmppa, 0, ctx.mdldat.nspobs);
        if (ctx.error.lfatal) return;
    }
    loadxr(ctx, /*toxreg=*/true);
    xrg_clear_working(ctx);
    // gtxreg.f:883-889: Nbx>0 -> Ixreg=1 (prior=yes -> 2, deferred); Havxtd ->
    // Axrgtd. editor.f:1723 clears Axrgtd if no TD group materialized.
    if (havtd) ctx.x11log.havxtd = true;
    if (ctx.xrgmdl.nbx > 0) ctx.hiddn.ixreg = 1;
    if (ctx.x11log.havxtd) ctx.x11log.axrgtd = true;
}

} // namespace x13
