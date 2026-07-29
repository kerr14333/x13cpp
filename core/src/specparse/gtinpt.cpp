// gtinpt.cpp -- top-level spec dispatch (gtinpt.f).
//
// M1 scope: faithful reproduction of the spec dispatch loop (getfcn over the
// spec-name dictionary), the cross-spec mutual-exclusion checks, and the
// end-of-input validation that determine the fatal/ok outcome. The exhaustive
// default-initialization of the estimation / X-11 / SEATS option COMMON blocks
// (~500 lines) is deferred to the milestones that consume them; here only the
// subset the parser reads back is initialized.
#include "specparse/specparse.hpp"
#include "notset.hpp"
#include "gen/model.hpp"
#include "numeric/numeric.hpp"   // dpeq (dpeq.f tolerance equality)
#include "composite/agr.hpp"      // agr1 (composite aggregation init)

#include <algorithm>
#include <cstring>

namespace x13 {

using namespace lexprm;

void gtinpt(X13Context& ctx, bool& lx11, bool& lseats, bool& lmodel, bool& inptok) {
    LexState& L = ctx.lex;

    // --- spec-name dictionary (verbatim from gtinpt.f) ---
    constexpr int PSPC = 20;
    static const char SPCDIC[] =
        "seriestransformidentifyregressionarimaautomdlestimateoutliercheck"
        "forecastx11historyslidingspanscompositex11regressionseatspickmdl"
        "forcemetadataspectrum";
    static const int spcptr[PSPC + 1] = {1, 7, 16, 24, 34, 39, 46, 54, 61, 66, 74,
        77, 84, 96, 105, 118, 123, 130, 135, 143, 151};
    int spclog[2 * PSPC];

    // Initialize parser input.
    intinp(ctx, ctx.units.mt);
    if (ctx.error.lfatal) return;
    setint(prm::NOTSET, 2 * PSPC, spclog);

    // Parser-read default subset.
    ctx.arima.begsrs(1) = 1;
    ctx.arima.begsrs(2) = 1;
    ctx.arima.nobs = 0;
    ctx.model.sp = 12;
    ctx.savcmn.svprec = 15;
    ctx.x11msc.yr2000 = true;
    ctx.x11opt.divpwr = prm::NOTSET;
    // gtinpt.f 323-336: x11 seasonal-adjustment option defaults (getx11 reads
    // back over these; unset options keep the default). Only the scalars the
    // ported X-11 spine consumes are set here -- title/taper/final args (Notc,
    // Thtapr, Fin*) have no ported consumer yet. Ny=Sp and Kersa=0 are set by
    // the editor-stage setup (editor.f:150,1486), mirrored in the run_x11 driver.
    ctx.x11opt.muladd = prm::NOTSET;
    ctx.x11opt.kfulsm = 0;
    ctx.x11opt.sigml = 1.5;
    ctx.x11opt.sigmu = 2.5;
    ctx.x11opt.lterm = prm::NOTSET;
    ctx.x11opt.ktcopt = 0;
    ctx.x11opt.imad = 0;
    ctx.x11opt.ishrnk = 0;
    ctx.x11opt.tic = 0.0;
    setint(0, 12, ctx.x11opt.lter.data());              // gtinpt.f:333 Lter=0
    ctx.xtrm.ksdev = 1;                                  // gtinpt.f:329 (NOT 0)
    setlg(false, 12, ctx.xtrm.csigvc.data());            // gtinpt.f:330 Csigvc(PSP)=F
    ctx.x11msc.shrtsf = false;                           // gtinpt.f:353
    ctx.x11msc.psuadd = false;                           // gtinpt.f:375
    ctx.x11msc.noxfct = false;                           // gtinpt.f:380
    ctx.x11msc.tru7hn = false;                           // gtinpt.f:381
    ctx.x11msc.lcentr = false;                           // gtinpt.f:382
    // gtinpt.f:354-371 -- the /rho/ spectrum defaults. These are set for EVERY
    // run, not only when a spectrum{} spec is present: x11ari.f:282-287 calls
    // spcdrv under a plain IF(Ny.eq.12), so the whole spectrum diagnostic block
    // is computed on every monthly run. They used to live at the head of
    // gt_spectrum, which meant a spec without spectrum{} read the struct's
    // zero-init instead (Ldecbl false, Spcsrs 0, Bgspec 0, ...).
    ctx.rho.spcdff = true;
    ctx.rho.spdfor = prm::NOTSET;
    ctx.rho.lstdff = false;
    ctx.rho.lfqalt = false;
    ctx.rho.llogqs = false;
    ctx.rho.lrbstsa = true;
    ctx.rho.lqchk = false;
    ctx.rho.lprsfq = false;
    ctx.rho.svallf = false;
    ctx.rho.ldecbl = true;
    ctx.rho.spctyp = 0;
    ctx.rho.spcsrs = 2;
    ctx.rho.mxarsp = prm::NOTSET;
    ctx.rho.ltk120 = true;
    ctx.rho.spclim = 6.0;
    ctx.rho.peakwd = prm::NOTSET;
    ctx.rho.plocal = 0.002;
    ctx.rho.bgspec(1) = prm::NOTSET;
    ctx.rho.bgspec(2) = prm::NOTSET;
    ctx.rho.axsame = false;
    ctx.model.isrflw = prm::NOTSET;
    ctx.missng.mvcode = -99999.0;
    ctx.missng.mvval = 1000000000.0;
    ctx.agr.iag = prm::NOTSET;
    ctx.agr.w = 1.0;
    ctx.arima.fcntyp = prm::NOTSET;
    ctx.arima.lam = 1.0;
    ctx.picktd.picktd = false;
    // gtinpt.f:407-411 -- the regression-effect "keep in the final series" flags.
    // Finao/Finls/Fintc/Finusr default FALSE (the zero-init already gives that);
    // Finhol defaults TRUE and is cleared at the parse tail when no holiday
    // regressor turned up (gtinpt.f:1240-1242). It is not a print-only flag: with
    // Finhol true x11pt2 folds Fachol into Faccal and x11pt3's `.not.Finhol`
    // guard skips the divide that would take it back out, so the holiday effect
    // stays in the combined calendar factor (D16) and is removed from D11/D13.
    // gtinpt.f:398-406 -- every Adj* indicator DEFAULTS TO 1 ("this effect is
    // removed"), and getreg.f:299-309 sets one to -1 for a `noapply=` type.
    // chkadj (x11drv.cpp) then normalises them against what the fitted model
    // actually carries, and its toggles are idempotent from either starting
    // value (`==1 && n==0 -> 0`, `==0 && n>0 -> 1`), which is why the port ran
    // this long on the struct's zero-init. What reads them BEFORE chkadj does
    // care: gtinpt.f:1252's Ldestm rule below.
    ctx.x11adj.adjtd = 1;  ctx.x11adj.adjhol = 1; ctx.x11adj.adjao = 1;
    ctx.x11adj.adjls = 1;  ctx.x11adj.adjtc = 1;  ctx.x11adj.adjso = 1;
    ctx.x11adj.adjsea = 1; ctx.x11adj.adjcyc = 1; ctx.x11adj.adjusr = 1;
    ctx.x11adj.finhol = true;
    ctx.prior.priadj = 0;    // gtinpt.f: Priadj=0 (no predefined prior adjustment)
    ctx.prior.kfmt = 0;      // gtinpt.f: Kfmt=0
    ctx.arima.reglom = 0;    // gtinpt.f: Reglom=0
    // gtinpt.f:170 -- Cnstnt=DNOTST (no transform{constant=}). Every consumer
    // keys on "!= DNOTST", so the struct's zero-init would wrongly read as a
    // constant of 0 and enter the removal branches.
    ctx.adj.cnstnt = prm::DNOTST;

    // gtinpt.f 259, 271-279: estimation-control defaults (an estimate{} spec
    // overrides these; that reader is a later M3 step). DFTOL etc. from model.prm.
    constexpr double DFTOL = 1e-5;
    // gtinpt.f:156 -- B defaults to DNOTST, i.e. "no initial value, estimate
    // me". regfix() keys on exactly that, and the user-regressor adrgef calls
    // in gt_regression pass B(idisp) through as their initial value, so a
    // zero-init here would read as a supplied coefficient of 0.
    setdp(prm::DNOTST, prm::PB, ctx.mdldat.b.data());
    // gtinpt.f:267-270. Convrg defaults TRUE and rgarma only UPDATES it behind
    // `Lestim .and. Nestpm > 0` (rgarma.f:387) -- so with every ARMA parameter
    // fixed there is nothing to estimate, nothing touches the flag, and the
    // default is what the .udg reports. Zero-init read as "did not converge".
    ctx.mdldat.convrg = true;
    ctx.mdldat.armaer = 0;
    // Nothing is fixed until regfix/mdlfix say otherwise.
    ctx.model.iregfx = 1;
    ctx.model.imdlfx = 1;
    ctx.model.nintvl = 0;              // gtinpt.f:259
    ctx.arima.mxiter = 1500;
    ctx.arima.mxnlit = 40;
    ctx.model.stepln = 0.0;
    ctx.model.tol = DFTOL;
    ctx.model.nltol0 = 100.0 * DFTOL;  // DFNLT0
    ctx.model.nltol = DFTOL;           // DFNLTL
    ctx.model.lextar = true;
    ctx.model.lextma = true;
    ctx.arima.lestim = true;

    // gtinpt.f 188-207 (+ mdlint.f): regression-group / ARIMA-operator state.
    intlst(prm::PB, ctx.model.colptr.data(), ctx.model.ncoltl);
    ctx.model.nb = ctx.model.ncoltl;
    ctx.model.ncxy = 1;
    intlst(prm::PGRP, ctx.model.grpptr.data(), ctx.model.ngrptl);
    intlst(prm::PGRP, ctx.model.grp.data(), ctx.model.ngrp);
    intlst(prm::POPR, ctx.model.opr.data(), ctx.model.nopr);
    intlst(prm::PMDL, ctx.model.mdl.data(), ctx.model.nmdl);
    intlst(prm::POPR, ctx.model.oprptr.data(), ctx.model.noprtl);   // mdlint.f
    ctx.model.mdl(2) = 1;    // Mdl(AR)=1
    ctx.model.mdl(3) = 1;    // Mdl(MA)=1
    ctx.arima.elong = true;  // gtinpt.f: Elong=T
    ctx.model.easidx = 0;    // gtinpt.f: Easidx=0
    ctx.extend.nfcst = prm::NOTSET;   // gtinpt.f: Nfcst=NOTSET
    ctx.extend.nbcst = prm::NOTSET;   // gtinpt.f: Nbcst=NOTSET
    ctx.arima.fctdrp = 0;             // gtinpt.f: Fctdrp=0
    ctx.arima.ciprob = 0.95;          // gtinpt.f: Ciprob=.95D0
    ctx.arima.lognrm = false;         // gtinpt.f: Lognrm=F
    ctx.arima.outest = prm::NOTSET;   // gtestm.f outest; gtinpt.f:1202 resolves
    ctx.arima.outamd = prm::NOTSET;   // gtautx.f outamd; same resolution
    // Outlier-identification defaults (gtinpt.f 304-421); gt_outlier overrides
    // when an outlier{} spec is present.
    ctx.arima.ltstao = false;
    ctx.arima.ltstls = false;
    ctx.arima.ltsttc = false;
    ctx.arima.ladd1 = true;
    ctx.arima.cvalfa = 0.05;          // gtinpt.f: Cvalfa=PT5 (PT5=0.05D0)
    ctx.arima.cvtype = false;         // gtinpt.f: Cvtype=F (Ljung, not corrected)
    ctx.arima.cvrduc = 0.5;           // gtinpt.f: Cvrduc=0.5D0
    for (int i = 1; i <= prm::POTLR; ++i)
        ctx.arima.critvl(i) = prm::DNOTST;   // setdp(DNOTST,...)
    ctx.model.tcalfa = prm::DNOTST;
    ctx.model.havtca = false;   // gtinpt.f:417 -- nothing has claimed tcrate yet
    ctx.model.havtca = false;   // gtinpt.f: Tcalfa=DNOTST
    ctx.arima.traicd = prm::DNOTST;   // gtinpt.f:293: Traicd=DNOTST (aicdiff);
                                      // editor.f defaults it to -2 (monthly/qtly)
    for (int i = 1; i <= 7; ++i)
        ctx.x11reg.dwt(i) = prm::DNOTST;   // gtinpt.f:470 setdp(DNOTST,7,Dwt)
    // gtinpt.f:457-458/478 -- x11regression{sigma= critical= cvalpha=}. The
    // first two must default to DNOTST, not to the struct's zero-init: 0.0 is a
    // MEANINGFUL value of critical= (gtxreg.f:311-313 reads it as "identify
    // outliers, but let editor derive the critical value"), so "not given" has
    // to be distinguishable from it.
    ctx.x11reg.sigxrg = prm::DNOTST;
    ctx.x11reg.critxr = prm::DNOTST;
    ctx.xrgmdl.cvxalf = 0.05;          // PT5
    ctx.picktd.tdzero = 0;
    ctx.picktd.lnzero = 0;
    setint(prm::NOTSET, 2, ctx.picktd.tddate.data());
    setint(prm::NOTSET, 2, ctx.picktd.lndate.data());

    // metadata{} defaults (gtinpt.f:553-557): no metadata present unless the
    // metadata{} block sets it.
    ctx.metadata.hvmtdt = false;
    ctx.metadata.nkey = 0;
    ctx.metadata.nval = 0;

    // Automatic-model-selection defaults (gtinpt.f 221-267). gt_automdl / gtauto
    // override these from an automdl{} spec; the automd driver reads them.
    ctx.arima.lautom = false;
    ctx.arima.lautod = false;
    ctx.arima.exdiff = 2;
    ctx.arima.hrinit = false;
    ctx.arima.bstdsn = std::string_view("");
    ctx.arima.bstdsn.data()[0] = prm::CNOTST;   // Bstdsn(1:1)=CNOTST
    ctx.arima.nbstds = 0;
    ctx.arima.ub1lim = 1.0 / 0.96;
    ctx.arima.ub2lim = 0.88;
    ctx.arima.ubfin = 1.05;
    ctx.arima.tsig = 1.0;
    ctx.arima.fct = 1.0 / (1.0 - 0.0125);
    ctx.arima.predcv = 0.14286;
    ctx.arima.cancel = 0.1;
    ctx.arima.pcr = 0.95;
    ctx.arima.lbalmd = false;
    ctx.arima.laccdf = false;
    ctx.arima.lotmod = true;
    setint(0, 2, ctx.arima.maxord.data());
    setint(prm::NOTSET, 2, ctx.arima.diffam.data());
    ctx.arima.frstar = 2;
    ctx.arima.lchkmu = true;
    ctx.arima.lmixmd = true;
    ctx.arima.lrejfc = false;
    ctx.arima.fctlm2 = 15.0;
    ctx.arima.lsovdf = false;

    // pickmdl{} defaults (gtinpt.f:248-257). gt_pickmdl / gtautx override them.
    // Autofl starts ALL BLANK here and gtautx.f:226 stamps CNOTST into its first
    // character only when no file= was given, which is the test automx.f:79
    // reads -- so a run with no pickmdl{} spec at all leaves it blank, and that
    // is never looked at because Lautox is false.
    ctx.arima.lautox = false;
    ctx.arima.pck1st = true;
    ctx.arima.id1st = true;
    ctx.arima.autofl = std::string_view("");
    ctx.arima.fctlim = 15.0;
    ctx.arima.bcklim = 18.0;
    ctx.arima.qlim = 5.0;
    ctx.arima.ovrdif = 0.9;

    // Control flags.
    bool havsrs = false, havesp = false, havotl = false, havreg = false, havtd = false;
    bool larma = false, hvfcst = false, hvspec = false, havmdl = false, havreq = false;
    bool lautom = false, lautox = false, hvmfil = false, lagr = false, l1stcomp = false;
    bool ldestm = false, x11reg = false;
    ctx.arima.ldestm = false;   // gtinpt.f:280
    bool ldata = false;
    std::string dtafil;
    lx11 = false; lseats = false; lmodel = false;
    inptok = true;

    // Local echo of transform state for the muladd/fcntyp end block; the real
    // Muladd lives on ctx.x11opt (getx11.f mode arg -> resolved below).

    int spcidx;
    while (true) {
        if (getfcn(ctx, SPCDIC, spcptr, PSPC, spcidx, spclog, inptok)) {
            switch (spcidx) {
            case 1:  // series
                getsrs(ctx, havsrs, havesp, lagr, ldata, dtafil, inptok);
                if (ctx.error.lfatal) return;
                // gtinpt.f:591-594 -- series{comptype=...} on the FIRST component
                // of a composite run (lagr) initializes the aggregation state
                // (agr1 with Iagr==0 zeroes the /agreg/ buffers and sets Iagr=1).
                if (lagr) {
                    agr1(ctx, ctx.arima.y.data(), ctx.arima.nobs);
                    ctx.x11agr = true;   // gtinpt.f:594
                }
                ctx.captured.spec_order.push_back("series");
                break;
            case 2:  // transform
                gt_transform(ctx, inptok);   // sets ctx.arima.fcntyp / lam (getadj.f)
                if (ctx.error.lfatal) return;
                ctx.captured.spec_order.push_back("transform");
                break;
            case 3:  // identify
                gt_identify(ctx, inptok);
                if (ctx.error.lfatal) return;
                if (!lmodel) lmodel = true;
                break;
            case 4:  // regression
                if (hvmfil) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify regression variables when a model file is given."); inptok = false; }
                gt_regression(ctx, havsrs, havesp, havtd, inptok);
                if (ctx.error.lfatal) return;
                if (!lmodel) lmodel = true;
                if (!havreg) havreg = true;
                if (!havreq) havreq = true;
                ctx.captured.spec_order.push_back("regression");
                break;
            case 5:  // arima
                if (lautom) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify arima and automdl spec in the same input file."); inptok = false; }
                else if (lautox) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify arima and pickmdl spec in the same input file."); inptok = false; }
                if (hvmfil) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify arima spec if model is read in from the file argument"); inptok = false; }
                gt_arima(ctx, inptok);
                if (ctx.error.lfatal) return;
                if (!lmodel) lmodel = true;
                if (!havmdl) havmdl = true;
                larma = true;
                ctx.captured.spec_order.push_back("arima");
                break;
            case 6:  // automdl
                if (larma) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify arima and automdl spec in the same input file."); inptok = false; }
                else if (lautox) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify automdl and pickmdl spec in the same input file."); inptok = false; }
                if (hvmfil) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify automdl spec if model is read in from the file argument"); inptok = false; }
                gt_automdl(ctx, inptok);
                if (ctx.error.lfatal) return;
                lautom = true;
                if (!lmodel) lmodel = true;
                if (!havmdl) havmdl = true;
                ldestm = true;
                ctx.captured.spec_order.push_back("automdl");
                break;
            case 7:  // estimate
                gt_estimate(ctx, inptok);
                if (ctx.error.lfatal) return;
                ldestm = true;
                if (!lmodel) lmodel = true;
                if (!havreq) havreq = true;
                ctx.captured.spec_order.push_back("estimate");
                break;
            case 8:  // outlier
                if (!havsrs) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Need to specify a series to identify outliers"); inptok = false; }
                gt_outlier(ctx, inptok);
                if (ctx.error.lfatal) return;
                ldestm = true;              // gtinpt.f:704
                if (!havreq) havreq = true;
                ctx.captured.spec_order.push_back("outlier");
                break;
            case 9:  // check
                gt_check(ctx, inptok);
                if (ctx.error.lfatal) return;
                ldestm = true;              // gtinpt.f:712
                if (!lmodel) lmodel = true;
                if (!havreq) havreq = true;
                break;
            case 10:  // forecast
                gt_forecast(ctx, inptok);
                if (ctx.error.lfatal) return;
                // gtinpt.f:721 -- note it reads hvfcst BEFORE this spec sets it,
                // so a bare forecast{} with neither maxlead nor maxback still
                // arms Ldestm only through Nfcst/Nbcst being explicit.
                if (ctx.extend.nfcst > 0 || ctx.extend.nbcst > 0 || hvfcst)
                    ldestm = true;
                if (!lmodel) lmodel = true;
                if (!havreq) havreq = true;
                hvfcst = true;
                ctx.captured.spec_order.push_back("forecast");
                break;
            case 11:  // x11
                if (lseats) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify x11 and seats spec in the same input file."); inptok = false; }
                gt_x11(ctx, havesp, inptok);
                if (ctx.error.lfatal) return;
                if (!lx11) lx11 = true;
                ctx.captured.spec_order.push_back("x11");
                break;
            case 14:  // composite
                if (ctx.agr.iagr <= 0) {
                    if (ctx.agr.iagr == 0)
                        inpter(ctx, PERROR, L.pos.data() + 1,
                               "No component series were specified for composite adjustment.");
                    else {
                        inpter(ctx, PERROR, L.errpos.data() + 1,
                               "Error(s) were found while executing the spec file(s) of component");
                        ctx.agr.iagr = prm::NOTSET;
                    }
                    inptok = false;
                }
                gt_composite(ctx, havsrs, havesp, lagr, inptok);
                if (ctx.error.lfatal) return;
                l1stcomp = true;
                ctx.captured.spec_order.push_back("composite");
                break;
            case 16:  // seats
                if (lx11) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify x11 and seats spec in the same input file."); inptok = false; }
                gt_seats(ctx, inptok);
                if (ctx.error.lfatal) return;
                ldestm = true;
                if (!lmodel) lmodel = true;
                if (!havreq) havreq = true;
                lseats = true;
                ctx.captured.spec_order.push_back("seats");
                break;
            case 18:  // force
                gt_force(ctx, inptok);
                if (ctx.error.lfatal) return;
                ctx.captured.spec_order.push_back("force");
                break;
            case 13:  // slidingspans
                gt_slidingspans(ctx, havesp, inptok);
                if (ctx.error.lfatal) return;
                // gtinpt.f:760 -- IF(.not.Ssdiff)Ssidif=Ssdiff: once Ssdiff
                // (additivesa=difference) is forced false->false is a no-op,
                // but if it went false due to additivesa=percent request
                // being overridden, force Ssidif false to match.
                if (!ctx.sspinp.ssdiff) ctx.sspinp.ssidif = ctx.sspinp.ssdiff;
                ctx.captured.spec_order.push_back("slidingspans");
                break;
            case 12:  // history
                gt_history(ctx, havesp, inptok);
                if (ctx.error.lfatal) return;
                if (ctx.rev.lrvfct || ctx.rev.lrvaic)
                    ldestm = true;          // gtinpt.f:753
                ctx.captured.spec_order.push_back("history");
                break;
            case 19:  // metadata
                gt_metadata(ctx, inptok);
                if (ctx.error.lfatal) return;
                ctx.captured.spec_order.push_back("metadata");
                break;
            case 15:  // x11regression
                gt_x11regression(ctx, havsrs, havesp, inptok);
                if (ctx.error.lfatal) return;
                ctx.captured.spec_order.push_back("x11regression");
                break;
            case 17:  // pickmdl
                gt_pickmdl(ctx, inptok);
                if (ctx.error.lfatal) return;
                // pickmdl provides the ARIMA model (via the candidate .mdl file),
                // so it satisfies the "model provision" check (gtinpt.f:877).
                if (!havmdl) havmdl = true;
                // gtinpt.f:878-881. gtautx.f:230 forces `iautom` to 1 when no
                // mode= was given, so this always fires -- `lautox` had been a
                // local nothing ever set, which is why it never did.
                lautox = ctx.arima.lautox;
                if (lautox) ldestm = true;
                ctx.model.imdlfx = 1;       // gtinpt.f:872
                ctx.captured.spec_order.push_back("pickmdl");
                break;
            case 20:  // spectrum
                gt_spectrum(ctx, inptok);
                if (ctx.error.lfatal) return;
                ctx.captured.spec_order.push_back("spectrum");
                break;
            default:
                inpter(ctx, PERROR, L.pos.data() + 1,
                       "This spec is recognized but not yet supported by the M1 parser port.");
                abend(ctx);
                return;
            }
            (void)havotl; (void)hvfcst; (void)hvspec; (void)havreg;
            // Ldestm: "this run estimates a model" (estimate/automdl/seats). It
            // is what gates the series{modelspan=} narrowing in arima.f:136.
            ctx.arima.ldestm = ldestm;
            (void)l1stcomp; (void)x11reg;
            continue;   // GO TO 210
        }
        if (!inptok) return;

        // --- End-of-input validation ---
        if (!havsrs) {
            writln(ctx, "ERROR: Series for analysis not specifed; a valid series or composite",
                   ctx.units.mt2, stdio::STDERR, true);
            writln(ctx, "       spec is required.", ctx.units.mt2, stdio::STDERR, false);
            inptok = false;
            return;
        }
        if (!havmdl && havreq) {
            writln(ctx, "ERROR: A spec that requires modeling was found ", ctx.units.mt2, stdio::STDERR, true);
            writln(ctx, "       (regression, check, estimate, forecast, outlier or seats),", ctx.units.mt2, stdio::STDERR, false);
            writln(ctx, "       but no provision for an ARIMA model was found.", ctx.units.mt2, stdio::STDERR, false);
            writln(ctx, "       If modeling was intended, please provide an ARIMA model ", ctx.units.mt2, stdio::STDERR, false);
            writln(ctx, "       using the arima spec or allow for automatic ARIMA", ctx.units.mt2, stdio::STDERR, false);
            writln(ctx, "       model selection using an automdl or pickmdl spec. ", ctx.units.mt2, stdio::STDERR, false);
            inptok = false;
            return;
        }

        // getx11.f:180-198 -- translate the x11{ mode } token into Muladd
        // (mult=0, add=1, logadd=2; pseudoadd=0 + Psuadd). The corpus uses full
        // tokens; match by prefix like gtdcvc against MODDIC='multaddlogaddpseudoadd'.
        if (!ctx.captured.x11_mode.empty()) {
            const std::string& m = ctx.captured.x11_mode;
            auto pre = [&](const char* s) {
                return m.size() <= std::strlen(s) && std::strncmp(m.c_str(), s, m.size()) == 0;
            };
            if (pre("pseudoadd")) { ctx.x11opt.muladd = 0; ctx.x11msc.psuadd = true; }
            else if (pre("logadd")) ctx.x11opt.muladd = 2;
            else if (pre("mult"))   ctx.x11opt.muladd = 0;
            else if (pre("add"))    ctx.x11opt.muladd = 1;
        }

        // Muladd / Fcntyp resolution (gtinpt.f:951-970). Only the Inptok-affecting
        // branch matters for the parse outcome; Tmpma feeds the X-11 spine.
        int fcntyp = ctx.arima.fcntyp;
        double lam = ctx.arima.lam;
        int muladd = ctx.x11opt.muladd;
        if (muladd == prm::NOTSET) {
            if (fcntyp == 0 || fcntyp == 4 || (fcntyp == 5 && dpeq(lam, 1.0))) {
                muladd = 1;
            } else {
                muladd = 0;
                if (fcntyp == prm::NOTSET) fcntyp = 4;
            }
        } else {
            if (fcntyp == 0) {
                writln(ctx, "ERROR: Cannot set seasonal adjustment mode when automatic transformation",
                       ctx.units.mt2, stdio::STDERR, true);
                writln(ctx, "       selection is done.", ctx.units.mt2, stdio::STDERR, false);
                inptok = false;
                return;
            } else if (fcntyp == prm::NOTSET) {
                fcntyp = 4;
            }
        }
        ctx.arima.fcntyp = fcntyp;
        ctx.x11opt.muladd = muladd;
        ctx.x11opt.tmpma = muladd;   // gtinpt.f:970 Tmpma=Muladd

        // editor.f:1484-1536 -- prior trading-day weight (x11regression tdprior)
        // resolution. Kswv=1 when any of the seven Dwt weights is nonzero; the
        // mult / log-additive path standardizes them to sum 7.0 (the only ported
        // branch -- additive / pseudo-additive tdprior and the Lxrneg negative-
        // weight clamp stay deferred). Resolved here at parse (Muladd is final) so
        // the pre-model estimation input (run_pre_model) and the X-11 fold (x11pt1)
        // both see the standardized weights.
        {
            x11reg_cmn& xr = ctx.x11reg;
            const bool psuadd = ctx.x11msc.psuadd;
            ctx.x11opt.kswv = 0;
            if (dpeq(xr.dwt(1), prm::DNOTST)) {
                for (int i = 1; i <= 7; ++i) xr.dwt(i) = 0.0;
            } else {
                for (int i = 1; i <= 7; ++i) {
                    // editor.f:1494-1500 -- a negative weight is meaningless for
                    // a multiplicative adjustment.
                    if (xr.dwt(i) < 0.0 && muladd == 0) {
                        writln(ctx, "ERROR: Prior Trading Day weights cannot be "
                               "less than zero for a", stdio::STDERR, ctx.units.mt2,
                               true);
                        writln(ctx, "       multiplicative seasonal adjustment.",
                               stdio::STDERR, ctx.units.mt2, false);
                        inptok = false;
                    } else if (!dpeq(xr.dwt(i), 0.0)) {
                        ctx.x11opt.kswv = 1;
                    }
                }
                if (ctx.x11opt.kswv == 1) {
                    if ((!psuadd && muladd == 0) || muladd == 2) {
                        double tmp = 0.0;
                        for (int i = 1; i <= 7; ++i) {
                            if (xr.dwt(i) < 0.0 && ctx.x11log.lxrneg) xr.dwt(i) = 0.0;
                            tmp += xr.dwt(i);
                        }
                        for (int i = 1; i <= 7; ++i) xr.dwt(i) *= 7.0 / tmp;
                    } else {
                        // editor.f:1518-1530 -- additive / pseudo-additive prior
                        // TD weights are REJECTED, not merely unported: the
                        // weights are a multiplicative device. Porting this is
                        // what makes the x11pt1 `muladd != 0` fatal unreachable
                        // for anything except a mode the oracle already refuses.
                        if (ctx.arima.fcntyp == 0) {
                            writln(ctx, "ERROR: Prior Trading Day weights cannot "
                                   "be specified when automatic", stdio::STDERR,
                                   ctx.units.mt2, true);
                            writln(ctx, "       transformation selection is "
                                   "performed.", stdio::STDERR, ctx.units.mt2,
                                   false);
                        } else {
                            writln(ctx, "ERROR: Prior Trading Day weights can only "
                                   "be specified for a", stdio::STDERR,
                                   ctx.units.mt2, true);
                            writln(ctx, "       multiplicative or log-additive "
                                   "seasonal adjustment.", stdio::STDERR,
                                   ctx.units.mt2, false);
                        }
                        inptok = false;
                    }
                } else if (muladd == 0) {
                    for (int i = 1; i <= 7; ++i) xr.dwt(i) = 1.0;
                }
            }
        }

        // --- gtinpt.f ~989-1067: td/lom prior-adjustment setup (Picktd) ---
        // With a log transform, the td (or td1coef) regressor's leap-year
        // component becomes a prior adjustment of the series: rmlnvr sets
        // Priadj (lpyear, or lom/loq for x11 type=trend) and removes any
        // Length-of-* / Leap Year regressor column.
        if (ctx.picktd.picktd) {
            if (dpeq(lam, 0.0)) {
                if (ctx.prior.priadj > 1) {
                    writln(ctx, "ERROR: A length of month, length of quarter, or "
                           "leap year prior adjustment", stdio::STDERR,
                           ctx.units.mt2, true);
                    writln(ctx, "       cannot be specified with the td or td1coef "
                           "regressor.", stdio::STDERR, ctx.units.mt2, false);
                    inptok = false;
                } else {
                    rmlnvr(ctx, ctx.prior.priadj, ctx.x11opt.kfulsm,
                           ctx.mdldat.nspobs);
                    if (ctx.error.lfatal) return;
                }
            } else {
                if (ctx.prior.priadj > 1) {
                    writln(ctx, "ERROR: A length of month, length of quarter, or "
                           "leap year prior adjustment", stdio::STDERR,
                           ctx.units.mt2, true);
                    writln(ctx, "       cannot be specified with the td or td1coef "
                           "regressor.", stdio::STDERR, ctx.units.mt2, false);
                    inptok = false;
                } else if (ctx.x11opt.kfulsm == 2) {
                    // gtinpt.f: CALL replyf() -- x11 type=trend path, not yet
                    // ported.
                    writln(ctx, "ERROR: x11 type=trend with td regressors "
                           "(replyf.f) not yet ported.", stdio::STDERR,
                           ctx.units.mt2, true);
                    abend(ctx);
                    return;
                }
            }
        }
        // gtinpt.f 1072-1084: check for lom in the regression and in the prior
        // adjustment.
        if (ctx.prior.priadj > 1 && ctx.model.nb > 0) {
            for (int icol = 1; icol <= ctx.model.nb; ++icol) {
                int t = ctx.model.rgvrtp(icol);
                if (t == 5 || t == 6 || t == 7 || t == 9 ||    // PRGTLM/LQ/LY/SL
                    t == 58 || t == 59 || t == 60) {           // PRGULM/ULQ/ULY
                    writln(ctx, "ERROR: Cannot have a length-of-period or leap "
                           "year regressor with a", stdio::STDERR, ctx.units.mt2,
                           true);
                    writln(ctx, "       length-of-period or leap year prior "
                           "adjustment.", stdio::STDERR, ctx.units.mt2, false);
                    inptok = false;
                    break;
                }
            }
        }

        // gtinpt.f:1240-1242 -- clear the Finhol default when nothing in the run
        // actually produces a holiday effect. Havhol is set by the regression
        // parsers (adpdrg.f:971/1037/1105 for easter/statcaneaster/stockeaster,
        // labor and thanksgiving; getreg.f:345 for a user holiday regressor;
        // getreg.f:851 for the aictest easter, Leastr), so it is equivalent to
        // scanning the built model for chkadj's Nhol regressor types.
        {
            // gtinpt.f:1239 -- Khol takes the x11{x11easter=} setting here, and
            // it is READ by the Finhol test on the next line: with the classic
            // X-11 Easter on (Khol==1) Finhol stays TRUE even though no holiday
            // REGRESSOR exists. That matters at x11pt3.f:525, where `.not.Finhol`
            // gates `Faccal /= Fachol` -- x11pt1 folds X11hol into Faccal and
            // x11pt2 folds the same factor into Fachol, so letting the divide run
            // cancels the Easter factor out of Faccal entirely (Faccal == 1) and
            // x11pt4's Tdbar/Vtd then report no calendar effect at all.
            ctx.x11opt.khol = ctx.x11opt.keastr;
            bool havhol = ctx.arima.leastr;
            for (int icol = 1; !havhol && icol <= ctx.model.nb; ++icol) {
                const int t = ctx.model.rgvrtp(icol);
                if (t == prm::PRGTEA || t == prm::PRGTLD || t == prm::PRGTTH ||
                    t == prm::PRGTEC || t == prm::PRGTES ||
                    (t >= prm::PRGTUH && t <= prm::PRGUH5))
                    havhol = true;
            }
            if (!(havhol || ctx.x11log.axrghl || ctx.x11log.axruhl ||
                  ctx.x11opt.khol == 1) && ctx.x11adj.finhol)
                ctx.x11adj.finhol = false;
        }

        // gtinpt.f:1242-1257. With no regression{} spec there is nothing to
        // remove, so every Adj* indicator is cleared (a no-op at parse time in
        // this port -- chkadj derives them from the fitted model later -- but
        // transcribed because the ELSE arm is not). That ELSE arm is the last
        // Ldestm rule: an X-11 run carrying a regression effect that has to be
        // removed from, or kept in, the final series must ESTIMATE the model
        // even though no estimate{}/forecast{}/outlier{} spec asked for one.
        // `generated/airline_user-reg-x11` is exactly that case -- a user
        // regressor plus `forecast{maxlead=0}`, which suppresses both of the
        // earlier rules (:721 sees Nfcst==0, :1151 sees Nfcst set).
        {
            auto& xa = ctx.x11adj;
            if (!havreg) {
                xa.adjtd = 0; xa.adjhol = 0; xa.adjao = 0; xa.adjls = 0;
                xa.adjtc = 0; xa.adjso = 0; xa.adjsea = 0; xa.adjcyc = 0;
                xa.adjusr = 0;
            } else if (!ldestm && lx11) {
                if (xa.adjtd > 0 || xa.adjhol > 0 || xa.adjao > 0 ||
                    xa.adjls > 0 || xa.adjtc > 0 || xa.adjso > 0 ||
                    xa.adjsea > 0 || xa.adjcyc > 0 || xa.adjusr > 0 ||
                    xa.finusr || xa.finao || xa.finls || xa.fintc ||
                    (!(ctx.x11log.axrghl || ctx.x11log.axruhl) && xa.finhol) ||
                    ctx.x11opt.khol == 1)
                    ldestm = true;
            }
        }

        // gtinpt.f:1282-1286 / gtspec.f:324-327 -- resolve Bgspec, the start of
        // the spectrum/QS diagnostic span: eight years (95 periods) back from
        // the end of the series span, clamped forward to the series start. The
        // oracle resolves it in gtspec's own tail when a spectrum{} spec was
        // present and in gtinpt's `hvspec` ELSE when it was not; the two
        // branches are identical for the default, so one site covers both.
        // It has to happen HERE rather than in run_spectrum, because genqs's
        // residual pair (arima.f:1110-1111) keys on it during estimation.
        if (ctx.rho.bgspec(1) == prm::NOTSET) {
            int endspn[2];
            addate(ctx.mdldat.begspn.data(), ctx.model.sp,
                   ctx.mdldat.nspobs - 1, endspn);
            int bgspec[2];
            addate(endspn, ctx.model.sp, -95, bgspec);
            int nspec = 0;
            dfdate(bgspec, ctx.mdldat.begspn.data(), ctx.model.sp, nspec);
            if (nspec < 0) {
                bgspec[0] = ctx.mdldat.begspn(1);
                bgspec[1] = ctx.mdldat.begspn(2);
            }
            ctx.rho.bgspec(1) = bgspec[0];
            ctx.rho.bgspec(2) = bgspec[1];
        }

        // gtinpt.f:1287-1290 / gtspec.f:355 -- resolve Peakwd. The two sites
        // are NOT equivalent: gtspec's is a bare `Peakwd=1` with the quarterly
        // override commented out, so a quarterly spec carrying a spectrum{}
        // block gets 1 where one without gets 3. Inert today (Peakwd is only
        // read by the monthly peak grid) but transcribed as written.
        if (ctx.rho.peakwd == prm::NOTSET) {
            ctx.rho.peakwd = 1;
            if (ctx.model.sp == 4 && !ctx.spcout.requested) ctx.rho.peakwd = 3;
        }

        // gtinpt.f:220: finalize the ARMA model dimensions for estimation
        // (Lar/Lma flags, Nintvl differencing order, Nextvl floor).
        mdlfin(ctx);

        // gtinpt.f 1110-1117: no forecasts excluded when seasonal adjustment done.
        if (lx11 && ctx.arima.fctdrp > 0) {
            writln(ctx, "WARNING: No observations should be excluded from "
                   "forecasting when a", ctx.units.mt2, stdio::STDERR, true);
            writln(ctx, "         seasonal adjustment is done.", ctx.units.mt2,
                   stdio::STDERR, false);
            ctx.arima.fctdrp = 0;
        }

        // gtinpt.f:1201: if a regARIMA model is estimated AND an irregular-
        // component x11regression was requested (Ixreg==1), promote to Ixreg==2 --
        // the TD/holiday regressors become a PRIOR adjustment (estimated by
        // xrgdrv's transparent SA and applied before the model), not an in-line
        // irregular regression. Drives run_pre_model's xrgdrv call + the main
        // x11pt1 Ixreg==3 fold.
        if (lmodel && ctx.hiddn.ixreg == 1) ctx.hiddn.ixreg = 2;

        // gtinpt.f 1142-1167: default Nbcst / Nfcst.
        if (ctx.extend.nbcst == prm::NOTSET) ctx.extend.nbcst = 0;
        if (ctx.extend.nfcst == prm::NOTSET) {
            if (lmodel) {
                if (lx11 || lseats) {
                    // gtinpt.f:1151 -- an adjustment run with a model estimates
                    // one, whether or not any spec asked for it explicitly.
                    if (!ldestm) ldestm = true;
                    if (lseats) ctx.extend.nfcst = std::max(12, 3 * ctx.model.sp);
                    else ctx.extend.nfcst = ctx.model.sp;
                } else if (hvfcst) {
                    ctx.extend.nfcst = ctx.model.sp;
                } else {
                    ctx.extend.nfcst = 0;
                }
            } else {
                ctx.extend.nfcst = 0;
            }
        }

        // The parse-tail rules above (gtinpt.f:1151) can still ARM Ldestm after
        // the dispatch loop has made its last copy, so re-publish it here.
        ctx.arima.ldestm = ldestm;

        // gtinpt.f:1203-1216 -- resolve the two out-of-sample switches. Each spec
        // supplies at most one (`estimate{outofsample=}` -> outest,
        // `pickmdl{outofsample=}` -> outamd), and whichever is given sets BOTH
        // flags; given both, each sets its own. Outfer is what the automatic
        // model search reads (amdfct's `Lauto` arm), Outfct what everything else
        // does. Needs Lmodel, so it lives here rather than in either reader.
        if (lmodel) {
            const int oe = ctx.arima.outest, oa = ctx.arima.outamd;
            if (oe == prm::NOTSET && oa == prm::NOTSET) {
                ctx.arima.outfct = false;
                ctx.arima.outfer = false;
            } else if (oe == prm::NOTSET) {
                ctx.arima.outfer = (oa == 1);
                ctx.arima.outfct = ctx.arima.outfer;
            } else if (oa == prm::NOTSET) {
                ctx.arima.outfct = (oe == 1);
                ctx.arima.outfer = ctx.arima.outfct;
            } else {
                ctx.arima.outfer = (oa == 1);
                ctx.arima.outfct = (oe == 1);
            }
        }

        // gtinpt.f:1170 -- a COMPONENT (Iagr 1 or 2; the total is 3) ANDs its own
        // Lx11 into X11agr. One SEATS component is enough to send the total's
        // indirect adjustment down agr3s rather than agr3.
        if (ctx.agr.iagr > 0 && ctx.agr.iagr < 3) ctx.x11agr = ctx.x11agr && lx11;

        // gtinpt.f 1172-1181: if an X-11 regression is done, set its forecast
        // horizon (Nfcstx) to at least one seasonal year. The transparent xrgdrv
        // pass (and the main run's Faccal fold at x11pt3) span [Pos1bk,Posffc]
        // using Nfcstx, so the calendar factor must extend a full year even when
        // the model forecast count is smaller.
        if (ctx.hiddn.ixreg > 0) {
            ctx.xrgfct.nfcstx = ctx.extend.nfcst;
            ctx.xrgfct.nbcstx = ctx.extend.nbcst;
            if (ctx.extend.nfcst < ctx.model.sp) ctx.xrgfct.nfcstx = ctx.model.sp;
        }
        return;
    }
}

// mdlfin -- gtinpt.f:220 model-finalize block (see specparse.hpp).
void mdlfin(X13Context& ctx) {
    auto& m = ctx.model;
    m.lar = m.lextar && m.mxarlg > 0;
    m.lma = m.lextma && m.mxmalg > 0;
    if (m.lextar) {
        m.nintvl = m.mxdflg;
        m.nextvl = m.mxarlg + m.mxmalg;
    } else {
        m.nintvl = m.mxdflg + m.mxarlg;
        m.nextvl = m.lextma ? m.mxmalg : 0;
    }
}

} // namespace x13
