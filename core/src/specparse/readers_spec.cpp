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

#include <cmath>
#include <string>
#include <vector>

namespace x13 {

using namespace lexprm;

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
                cap->push_back(cur_tok(ctx));
            lex(ctx);
        }
        lex(ctx);   // consume the closing bracket
    } else if (L.nxtktp == DBL || L.nxtktp == INTGR || L.nxtktp == NAME ||
               L.nxtktp == QUOTE) {
        if (cap) cap->push_back(cur_tok(ctx));
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
    while (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
        if (ctx.error.lfatal) return;
        // Capture the value tokens for the args whose state we set here:
        //   6 = adjust, 8 = power, 9 = function, 11 = save, 17 = aicdiff.
        std::vector<std::string> cap;
        bool want = (argidx == 6 || argidx == 8 || argidx == 9 || argidx == 11 ||
                     argidx == 17);
        consume_value(ctx, want ? &cap : nullptr);
        if (ctx.error.lfatal) return;
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
        } else {                     // title / diff / ar / ma (gtinvl gated)
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
void gt_x11(X13Context& ctx, bool& inptok) {
    constexpr int PARG = 25;
    static const char ARGDIC[] =
        "modesigmalimseasonalmatrendmatitleextremeadjtypeappendfcsttrendic"
        "calendarsigmasigmavecx11eastertaperkeepholidayfinalsfshortprintsave"
        "savelogprint1stpassexcludefcsttrue7termshrinkcenterseasonalappendbcst";
    static const int argptr[PARG + 1] = {1, 5, 13, 23, 30, 35, 45, 49, 59, 66, 79,
        87, 96, 101, 112, 117, 124, 129, 133, 140, 152, 163, 172, 178, 192, 202};
    int arglog[2 * PARG];
    for (auto& v : arglog) v = -32767;
    int argidx;
    while (gtarg(ctx, ARGDIC, argptr, PARG, argidx, arglog, inptok)) {
        if (ctx.error.lfatal) return;
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
        } else {
            std::vector<std::string>* cap = nullptr;
            std::vector<std::string> tmp;
            if (argidx == 9 || argidx == 10) cap = &tmp;   // 9=save, 10=aictest
            consume_value(ctx, cap);
            if (ctx.error.lfatal) return;
            if (argidx == 9)
                for (const auto& t : tmp) ctx.captured.save_tables.push_back(t);
            if (argidx == 10) ctx.captured.aictest_vars = tmp;
        }
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

// ---- composite{} (getcmp.f) : consume args; component aggregation deferred --
void gt_composite(X13Context& ctx, bool& havsrs, bool& lagr, bool& inptok) {
    (void)havsrs; (void)lagr;
    constexpr int PARG = 13;
    static const char ARGDIC[] =
        "nametitleprintsavedecimalsmodelspansaveprecisionsavelogyr2000"
        "indoutlierappendfcstappendbcsttype";
    static const int argptr[PARG + 1] = {1, 5, 10, 15, 19, 27, 36, 49, 56, 62, 72,
        82, 92, 96};
    gt_generic(ctx, ARGDIC, argptr, PARG, inptok);
}

} // namespace x13
