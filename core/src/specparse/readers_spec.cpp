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
    gt_generic(ctx, ARGDIC, argptr, PARG, inptok);

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

// ---- seats{} (gtseat.f) ----------------------------------------------------
void gt_seats(X13Context& ctx, bool& inptok) {
    constexpr int PARG = 22;
    static const char ARGDIC[] =
        "printsavesavelogappendfcstnoadmissimeanqmaxoutepsphixlrmodepsiv"
        "maxithplanhpcyclestatseastabtablesbiasfiniteprintphtrfhptargethprmls";
    static const int argptr[PARG + 1] = {1, 6, 10, 17, 27, 35, 40, 44, 47, 53, 55,
        59, 64, 69, 74, 81, 89, 98, 102, 108, 118, 126, 132};
    gt_generic(ctx, ARGDIC, argptr, PARG, inptok);
    ctx.captured.has_seats = true;
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
