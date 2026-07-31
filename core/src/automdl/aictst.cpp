// aictst.cpp -- tdaic.f / easaic.f / addtd.f / addeas.f (see aictst.hpp).
//
// Faithful translations of the automdl trading-day and Easter AIC tests. All
// WRITE/savelog/summary output and the mktdlb/mkealb label builders are deferred
// (Lprt=false, Lsavlg=false, Lsumm=0 on the automd call sites). Group titles the
// tests search on with strinx are written by addtd/addeas (built here with
// std::to_string, byte-identical to the oracle's itoc output).
#include "automdl/aictst.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "numeric/numeric.hpp"       // dpeq, chsppf
#include "regarima/estimate.hpp"     // rgarma, prlkhd
#include "automdl/automd_finalize.hpp"  // ssprep_save, restor_model
#include "regarima/regvar.hpp"       // regvar, td7var, gtrgpt
#include "transform/transform.hpp"   // trnfcn
#include "automdl/iddiff.hpp"        // prterr
#include "regarima/outlier.hpp"      // wrtdat (regime-date title, addtd)
#include "specparse/specparse.hpp"   // strinx, adrgef, dlrgef, copy, abend
#include "gen/srslen.hpp"            // prm::PLEN
#include "gen/model.hpp"             // prm PRG* type codes, error codes, PTDAIC
#include "gen/notset.hpp"            // prm::DNOTST, prm::NOTSET

namespace x13 {

namespace {
using namespace prm;

// eltfcn operation selectors (tdaic uses DIV / MULT) and Priadj codes.
constexpr int DIV = 4, MULT = 3, PLOM = 2, PLOQ = 3;

// A hard estimation error discontinues the AIC test (tdaic.f:358 / easaic.f:152).
bool armaer_is_fatal(int e) {
    return e == PMXIER || e == PSNGER || e == PISNER || e == PNIFER ||
           e == PNIMER || e == PCNTER || e == POBFN0 || e < 0;
}

// True when Rgvrtp(begcol) names a trading-day / length-of-month regressor that
// the trading-day AIC test manages (tdaic.f:149-161). lomtst gates the
// length-of-month/quarter/leap-year family (kept when a separate lomaic test
// owns them).
bool is_td_rgvr(int rt, int lomtst) {
    bool base = rt == PRGTST || rt == PRGTTD || rt == PRRTST || rt == PRRTTD ||
                rt == PRATST || rt == PRATTD;
    bool lom = lomtst == 0 &&
               (rt == PRGTLM || rt == PRGTLQ || rt == PRGTLY || rt == PRGTSL ||
                rt == PRRTLM || rt == PRRTLQ || rt == PRRTLY || rt == PRRTSL ||
                rt == PRATLM || rt == PRATLQ || rt == PRATLY);
    bool one = rt == PRATSL || rt == PRG1TD || rt == PRR1TD || rt == PRA1TD ||
               rt == PRG1ST || rt == PRR1ST || rt == PRA1ST;
    return base || lom || one;
}

// The narrower list used at the "remove trading day" epilogue when a specific
// TD model was chosen (tdaic.f:569-575) -- no length-of-month family.
bool is_td_rgvr_narrow(int rt) {
    return rt == PRGTST || rt == PRGTTD || rt == PRRTST || rt == PRRTTD ||
           rt == PRATST || rt == PRATTD || rt == PRATSL || rt == PRG1TD ||
           rt == PRR1TD || rt == PRA1TD || rt == PRG1ST || rt == PRR1ST ||
           rt == PRA1ST;
}

// strinx over the four trading-day group titles (tdaic.f:133-141).
int find_td_group(model_cmn& m) {
    int g = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                   "Trading Day");
    if (g == 0)
        g = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                   "Stock Trading Day");
    if (g == 0)
        g = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                   "1-Coefficient Trading Day");
    if (g == 0)
        g = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                   "1-Coefficient Stock Trading Day");
    return g;
}
}  // namespace

// ---------------------------------------------------------------------------
// addtd.f
// ---------------------------------------------------------------------------
void addtd(X13Context& ctx, int aicstk, const int* aicrgm, int aictd0, int sp,
           int tdindx) {
    if (tdindx == 0) return;

    std::string datstr;
    bool hasrgm = aicrgm[0] != NOTSET;
    if (hasrgm) datstr = wrtdat(aicrgm, sp);
    if (ctx.error.lfatal) return;

    static const char* day[6] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    std::string tgrptl;
    int vartd, vartd1, vartd2, nvar;
    if (tdindx == 3 || tdindx == 6) {
        tgrptl = "Stock Trading Day[" + std::to_string(aicstk) + "]";
        if (tdindx == 6) {
            vartd = PRG1ST; vartd1 = PRR1ST; vartd2 = PRA1ST; nvar = 1;
        } else {
            vartd = PRGTST; vartd1 = PRRTST; vartd2 = PRATST; nvar = 6;
        }
    } else {
        tgrptl = "Trading Day";
        if (tdindx == 4 || tdindx == 5) {
            vartd = PRG1TD; vartd1 = PRR1TD; vartd2 = PRA1TD; nvar = 1;
        } else {
            vartd = PRGTTD; vartd1 = PRRTTD; vartd2 = PRATTD; nvar = 6;
        }
    }

    // Group-title prefix for the one-coefficient families (addtd.f:76,97,110).
    auto onepfx = [](const std::string& s) { return "1-Coefficient " + s; };

    if (aictd0 == 0) {
        std::string gttl = tgrptl;
        if (hasrgm) gttl = tgrptl + " (after " + datstr + ")";
        for (int i = 1; i <= nvar; ++i) {
            if (nvar == 1)
                adrgef(ctx, DNOTST, "Weekday", onepfx(gttl), vartd, false, false);
            else
                adrgef(ctx, DNOTST, day[i - 1], gttl, vartd, false, false);
            if (ctx.error.lfatal) return;
        }
    }
    if (hasrgm) {
        if (aictd0 >= 0) {
            std::string gttl;
            if (aictd0 == 0)
                gttl = tgrptl + " (change for before " + datstr + ")";
            else
                gttl = tgrptl + " (before " + datstr + ")";
            if (nvar == 1) {
                adrgef(ctx, DNOTST, "Weekday I", onepfx(gttl), vartd1, false, false);
                if (ctx.error.lfatal) return;
            } else {
                for (int i = 1; i <= nvar; ++i) {
                    adrgef(ctx, DNOTST, std::string(day[i - 1]) + " I", gttl,
                           vartd1, false, false);
                    if (ctx.error.lfatal) return;
                }
            }
        } else {
            std::string gttl = tgrptl + " (starting " + datstr + ")";
            if (nvar == 1) {
                adrgef(ctx, DNOTST, "Weekday II", onepfx(gttl), vartd2, false,
                       false);
                if (ctx.error.lfatal) return;
            } else {
                for (int i = 1; i <= nvar; ++i) {
                    adrgef(ctx, DNOTST, std::string(day[i - 1]) + " II", gttl,
                           vartd2, false, false);
                    if (ctx.error.lfatal) return;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// addeas.f
// ---------------------------------------------------------------------------
void addeas(X13Context& ctx, int keastr, int easidx, int eastst) {
    std::string tgrptl;
    int etype;
    if (easidx == 0) {
        if (eastst == 1) {
            tgrptl = "Easter[";
            etype = PRGTEA;
        } else {
            tgrptl = "StockEaster[";
            etype = PRGTES;
        }
        tgrptl += std::to_string(keastr);
    } else {
        tgrptl = "StatCanEaster[";
        tgrptl += std::to_string(keastr - easidx);
        etype = PRGTEC;
    }
    if (ctx.error.lfatal) return;
    tgrptl += "]";
    adrgef(ctx, DNOTST, tgrptl, tgrptl, etype, false, false);
}

// ---------------------------------------------------------------------------
// tdaic.f
// ---------------------------------------------------------------------------
void tdaic(X13Context& ctx, double* trnsrs, double* a, int& nefobs, int& na,
           int& frstry, int& tdmdl1, bool ltdlom, bool& lester, bool lsumm) {
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& ar = ctx.arima;
    auto& pk = ctx.picktd;
    auto& pr = ctx.prior;
    auto& aj = ctx.adj;
    auto& ext = ctx.extend;
    auto& pu = ctx.priusr;
    auto& pad = ctx.priadj;
    auto& ip = ctx.inpt;

    constexpr double ONE = 1.0;

    // ---- store initial model values (tdaic.f:61-72) ----
    std::vector<double> lomeff(PLEN, ONE);
    std::vector<double> a2(PLEN);
    copy(aj.adj.data(), PLEN, 1, a2.data());
    int irgfx = m.iregfx;
    bool pktd = pk.picktd;
    int kf2 = pr.kfmt;
    int ilom = pr.priadj;
    double aictd = DNOTST;

    std::vector<double> tsrs(PLEN);
    copy(trnsrs, PLEN, 1, tsrs.data());

    // Indicator: is a trading-day effect already in the default model?
    tdmdl1 = find_td_group(m);

    // Length-of-month effect for possible later use (tdaic.f:119-124).
    std::unique_ptr<bool[]> begrgm_buf(new bool[PLEN]);
    bool* begrgm = begrgm_buf.get();
    if (pk.lrgmtd && (pk.tdzero % 2) != 0)
        gtrgpt(ctx, aj.begadj.data(), pk.tddate.data(), pk.tdzero, begrgm, aj.nadj);
    else
        for (int i = 0; i < PLEN; ++i) begrgm[i] = true;
    bool lom = (pr.priadj == PLOM || pr.priadj == PLOQ);
    td7var(aj.begadj.data(), m.sp, aj.nadj, 1, 1, lom, false, true,
           lomeff.data(), begrgm);

    double aicno = DNOTST;
    int nbno = 0, nbtd = 0;

    // tdaic.f:88-103 -- the savelog header. `td.reg` is the label of the
    // REQUESTED test (Itdtst) and `td.reg2` that of the LAST candidate, and the
    // latter is written only for a three-entry vector -- the editor's
    // "td plus its 1-coefficient sibling" pair.
    if (lsumm) {
        auto& sv = ctx.aictest_log;
        sv.ran = true;
        sv.td_aicc.clear();
        sv.td_num = ar.ntdvec - 1;
        sv.td_reg = mktdlb(ctx, ar.itdtst, ar.aicstk, pk.tddate.data(), pk.tdzero,
                           m.sp);
        if (ctx.error.lfatal) return;
        sv.td_reg2.clear();
        if (ar.ntdvec == 3) {
            sv.td_reg2 = mktdlb(ctx, ar.tdayvc(ar.ntdvec), ar.aicstk,
                                pk.tddate.data(), pk.tdzero, m.sp);
            if (ctx.error.lfatal) return;
        }
    }

    // ---- loop through the TD model choices ----
    for (int i = 1; i <= ar.ntdvec; ++i) {
        int thisTD = ar.tdayvc(i);

        // Delete any TD regressor already present (tdaic.f:133-166).
        int tdgrp = find_td_group(m);
        if (tdgrp > 0) {
            for (int igrp = m.ngrp; igrp >= 1; --igrp) {
                int begcol = m.grp(igrp - 1);
                int ncol = m.grp(igrp) - begcol;
                if (is_td_rgvr(m.rgvrtp(begcol), ar.lomtst)) {
                    dlrgef(ctx, begcol, ar.nrxy, ncol);
                    if (ctx.error.lfatal) return;
                }
            }
        }

        if (i == 1) {
            // If Picktd, put length of month back in the series (log only).
            if (pktd) {
                pk.picktd = false;
                pr.priadj = 1;
                if (dpeq(ar.lam, 0.0)) {
                    if (pu.nustad > 0) {
                        eltfcn(DIV, &ar.y(ar.frstsy), &pad.usrtad(pu.frstat),
                               ext.nobspf, trnsrs);
                        copy(&pad.usrtad(pu.frstat), aj.nadj, 1, &aj.adj(1));
                    } else {
                        copy(&ar.y(ar.frstsy), ext.nobspf, -1, trnsrs);
                    }
                    if (pu.nuspad > 0) {
                        eltfcn(DIV, &ar.y(ar.frstsy), &pad.usrpad(pu.frstap),
                               ext.nobspf, trnsrs);
                        if (pu.nustad > 0)
                            eltfcn(MULT, &aj.adj(1), &pad.usrpad(pu.frstap),
                                   aj.nadj, &aj.adj(1));
                        else
                            copy(&pad.usrpad(pu.frstap), aj.nadj, 1, &aj.adj(1));
                    } else {
                        setdp(1.0, PLEN, aj.adj.data());
                    }
                    int ntrn = (m.lmvaft || m.ln0aft) ? d.nspobs : ext.nobspf;
                    trnfcn(ctx, trnsrs, ntrn, ar.fcntyp, ar.lam, trnsrs);
                    if (ctx.error.lfatal) return;
                }
            }
        } else if (i == 2) {
            if (tdmdl1 == 0) {
                if (thisTD == 1 || thisTD == 4) {
                    pk.picktd = true;
                    if (ar.fcntyp == 4 || dpeq(ar.lam, 1.0)) {
                        if (ltdlom) {
                            if (m.sp == 12)
                                adrgef(ctx, DNOTST, "Length-of-Month",
                                       "Length-of-Month", PRGTLM, false, false);
                            else if (m.sp == 4)
                                adrgef(ctx, DNOTST, "Length-of-Quarter",
                                       "Length-of-Quarter", PRGTLQ, false, false);
                        } else {
                            adrgef(ctx, DNOTST, "Leap Year", "Leap Year", PRGTLY,
                                   false, false);
                        }
                        if (ctx.error.lfatal) return;
                    } else {
                        if (ltdlom) {
                            if (m.sp == 12) pr.priadj = PLOM;
                            else if (m.sp == 4) pr.priadj = PLOQ;
                        } else {
                            pr.priadj = 4;
                        }
                    }
                }
                m.iregfx = 0;

                // Length-of-month adjust the original series (power transform).
                if ((ilom <= 1 && pk.picktd) &&
                    !(ar.fcntyp == 4 || dpeq(ar.lam, 1.0))) {
                    int ntrn = (m.lmvaft || m.ln0aft) ? d.nspobs : ext.nobspf;
                    if (kf2 == 0) {
                        eltfcn(DIV, &ar.y(ar.frstsy), (lomeff.data() + (aj.adj1st - 1)),
                               ext.nobspf, trnsrs);
                        trnfcn(ctx, trnsrs, ntrn, ar.fcntyp, ar.lam, trnsrs);
                        if (ctx.error.lfatal) return;
                        copy(lomeff.data(), PLEN, 1, aj.adj.data());
                        pr.kfmt = 1;
                    } else {
                        eltfcn(DIV, &ar.y(ar.frstsy), (lomeff.data() + (aj.adj1st - 1)),
                               ext.nobspf, trnsrs);
                        eltfcn(DIV, trnsrs, &aj.adj(aj.adj1st), ext.nobspf,
                               trnsrs);
                        trnfcn(ctx, trnsrs, ntrn, ar.fcntyp, ar.lam, trnsrs);
                        if (ctx.error.lfatal) return;
                        eltfcn(MULT, (lomeff.data() + (aj.adj1st - 1)), &aj.adj(aj.adj1st),
                               ext.nobspf, &aj.adj(aj.adj1st));
                    }
                }
            } else {
                // Restore variables from the original model (tdaic.f:277-305).
                copy(a2.data(), PLEN, 1, aj.adj.data());
                pk.picktd = pktd;
                pr.kfmt = kf2;
                pr.priadj = ilom;
                copy(tsrs.data(), PLEN, 1, trnsrs);
                if ((thisTD == 1 || thisTD == 4) && pk.picktd) {
                    if (ar.fcntyp == 4 || dpeq(ar.lam, 1.0)) {
                        if (ltdlom) {
                            if (m.sp == 12)
                                adrgef(ctx, DNOTST, "Length-of-Month",
                                       "Length-of-Month", PRGTLM, false, false);
                            else if (m.sp == 4)
                                adrgef(ctx, DNOTST, "Length-of-Quarter",
                                       "Length-of-Quarter", PRGTLQ, false, false);
                        } else {
                            adrgef(ctx, DNOTST, "Leap Year", "Leap Year", PRGTLY,
                                   false, false);
                        }
                        if (ctx.error.lfatal) return;
                    }
                }
            }
        } else if (i == 3) {
            if (thisTD == 4) {
                if (ar.fcntyp == 4 || dpeq(ar.lam, 1.0)) {
                    if (ltdlom) {
                        if (m.sp == 12)
                            adrgef(ctx, DNOTST, "Length-of-Month",
                                   "Length-of-Month", PRGTLM, false, false);
                        else if (m.sp == 4)
                            adrgef(ctx, DNOTST, "Length-of-Quarter",
                                   "Length-of-Quarter", PRGTLQ, false, false);
                    } else {
                        adrgef(ctx, DNOTST, "Leap Year", "Leap Year", PRGTLY,
                               false, false);
                    }
                    if (ctx.error.lfatal) return;
                }
            }
        }

        // Add the new trading-day regressor (tdaic.f:327-336).
        if (i > 1 || tdgrp > 0) {
            if (i > 1) {
                addtd(ctx, ar.aicstk, pk.tddate.data(), pk.tdzero, m.sp, thisTD);
                if (ctx.error.lfatal) return;
            }
            regvar(ctx, trnsrs, ext.nobspf, ar.fctdrp, ext.nfcst, 0,
                   ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, pr.priadj,
                   ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
            if (ctx.error.lfatal) return;
        }

        // Reset user-seeded ARMA initial values (tdaic.f:341-346).
        if (m.nopr > 0) {
            int endlag = m.opr(m.nopr) - 1;
            for (int ilag = 1; ilag <= endlag; ++ilag)
                if (!m.arimaf(ilag)) d.arimap(ilag) = m.ap1(ilag);
        }

        // Estimate the model.
        bool argok = ar.lautom || ar.lautox;
        rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs, argok);
        if (!ctx.error.lfatal && (ar.lautom || ar.lautox) && !argok) lester = true;
        if (ctx.error.lfatal) return;
        if (armaer_is_fatal(d.armaer) ||
            ((ar.lautom || ar.lautox) && !argok)) {
            lester = true;
            return;
        }
        if (d.armaer != 0) d.armaer = 0;

        // Likelihood statistics -> Aicc (ctx.lkhd.aicc).
        prlkhd(ctx, &ar.y(ar.frstsy), &aj.adj(aj.adj1st), aj.adjmod, ar.fcntyp,
               ar.lam);
        if (ctx.error.lfatal) return;
        double aicc = ctx.lkhd.aicc;
        if (std::getenv("X13_AICDBG"))
            std::fprintf(stderr,
                         "[tdaic] i=%d thisTD=%d nb=%d convrg=%d aicc=%.10f\n", i,
                         thisTD, m.nb, (int)d.convrg, aicc);

        // tdaic.f:393/:399 -- one row per candidate, keyed by that candidate's
        // OWN label (mktdlb is re-run per i at :328), with i==1 the no-TD fit.
        if (lsumm) {
            std::string lab = "notd";
            if (i > 1) {
                lab = mktdlb(ctx, thisTD, ar.aicstk, pk.tddate.data(), pk.tdzero,
                             m.sp);
                if (ctx.error.lfatal) return;
            }
            ctx.aictest_log.td_aicc.push_back({lab, aicc});
        }

        if (i == 1) {
            aicno = aicc;
            nbno = m.nb;
        } else {
            if (i == 2) {
                ar.aicint = thisTD;
                aictd = aicc;
                if (!dpeq(ar.pvaic, DNOTST)) nbtd = m.nb;
            } else {
                if (!dpeq(ar.pvaic, DNOTST)) {
                    int aicdf = nbtd - m.nb;
                    double thiscv = chsppf(ar.pvaic, aicdf);
                    ar.rgaicd(PTDAIC) = thiscv - 2.0 * static_cast<double>(aicdf);
                }
                ar.dfaict = aicc - aictd;
                if (!(ar.dfaict > ar.rgaicd(PTDAIC))) {
                    ar.aicint = thisTD;
                    aictd = aicc;
                    if (!dpeq(ar.pvaic, DNOTST)) nbtd = m.nb;
                }
            }
        }
    }

    // ---- decide TD vs no-TD (tdaic.f:432-443) ----
    ar.dfaict = aicno - aictd;
    if (!dpeq(ar.pvaic, DNOTST)) {
        int aicdf = nbtd - nbno;
        double thiscv = chsppf(ar.pvaic, aicdf);
        ar.rgaicd(PTDAIC) = thiscv - 2.0 * static_cast<double>(aicdf);
    }
    if (ar.dfaict > ar.rgaicd(PTDAIC)) {
        // keep the best TD model
    } else {
        ar.aicint = 0;
    }

    // ---- rebuild the chosen model (tdaic.f:475-598) ----
    bool argok = ar.lautom || ar.lautox;
    if (ar.aicint == 0) {
        if (pktd) {
            pk.picktd = false;
            pr.priadj = 1;
            if (dpeq(ar.lam, 0.0)) {
                if (pu.nustad > 0) {
                    eltfcn(DIV, &ar.y(ar.frstsy), &pad.usrtad(pu.frstat),
                           ext.nobspf, trnsrs);
                    copy(&pad.usrtad(pu.frstat), aj.nadj, 1, &aj.adj(1));
                } else {
                    copy(&ar.y(ar.frstsy), ext.nobspf, -1, trnsrs);
                }
                if (pu.nuspad > 0) {
                    eltfcn(DIV, &ar.y(ar.frstsy), &pad.usrpad(pu.frstap),
                           ext.nobspf, trnsrs);
                    if (pu.nustad > 0)
                        eltfcn(MULT, &aj.adj(1), &pad.usrpad(pu.frstap), aj.nadj,
                               &aj.adj(1));
                    else
                        copy(&pad.usrpad(pu.frstap), aj.nadj, 1, &aj.adj(1));
                } else {
                    setdp(1.0, PLEN, aj.adj.data());
                }
                int ntrn = (m.lmvaft || m.ln0aft) ? d.nspobs : ext.nobspf;
                trnfcn(ctx, trnsrs, ntrn, ar.fcntyp, ar.lam, trnsrs);
                if (ctx.error.lfatal) return;
            }
        } else {
            copy(a2.data(), PLEN, 1, aj.adj.data());
            pr.kfmt = kf2;
            pk.picktd = pktd;
            m.iregfx = irgfx;
            if (ilom <= 1) {
                pr.priadj = ilom;
                copy(tsrs.data(), PLEN, 1, trnsrs);
            }
        }
        // Remove trading-day variables.
        for (int igrp = m.ngrp; igrp >= 1; --igrp) {
            int begcol = m.grp(igrp - 1);
            int ncol = m.grp(igrp) - begcol;
            if (is_td_rgvr(m.rgvrtp(begcol), ar.lomtst)) {
                dlrgef(ctx, begcol, ar.nrxy, ncol);
                if (ctx.error.lfatal) return;
            }
        }
        if (m.nopr > 0) {
            int endlag = m.opr(m.nopr) - 1;
            for (int ilag = 1; ilag <= endlag; ++ilag)
                if (!m.arimaf(ilag)) d.arimap(ilag) = m.ap1(ilag);
        }
        regvar(ctx, trnsrs, ext.nobspf, ar.fctdrp, ext.nfcst, 0,
               ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, pr.priadj,
               ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
        rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs, argok);
        if (!ctx.error.lfatal && (ar.lautom || ar.lautox) && !argok) abend(ctx);
        if (ctx.error.lfatal) return;
    } else if (ar.aicint != ar.tdayvc(ar.ntdvec)) {
        for (int igrp = m.ngrp; igrp >= 1; --igrp) {
            int begcol = m.grp(igrp - 1);
            int ncol = m.grp(igrp) - begcol;
            if (is_td_rgvr_narrow(m.rgvrtp(begcol))) {
                dlrgef(ctx, begcol, ar.nrxy, ncol);
                if (ctx.error.lfatal) return;
            }
        }
        addtd(ctx, ar.aicstk, pk.tddate.data(), pk.tdzero, m.sp, ar.aicint);
        if (m.nopr > 0) {
            int endlag = m.opr(m.nopr) - 1;
            for (int ilag = 1; ilag <= endlag; ++ilag)
                if (!m.arimaf(ilag)) d.arimap(ilag) = m.ap1(ilag);
        }
        regvar(ctx, trnsrs, ext.nobspf, ar.fctdrp, ext.nfcst, 0,
               ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, pr.priadj,
               ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
        rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs, argok);
        if (!ctx.error.lfatal && (ar.lautom || ar.lautox) && !argok) abend(ctx);
        if (ctx.error.lfatal) return;
    }

    // ---- prior-factor bookkeeping (tdaic.f:600-623) ----
    if (!pr.lpradj && pr.kfmt == 1) pr.lpradj = true;
    if (ar.aicint == 0) {
        if (pktd && !pk.picktd) {
            // tdaic.f:603/611 -- Sprior takes the prior the Picktd
            // transition just put in Adj. DEAD IN THIS PORT: setpri is
            // assigned only in x11_prestage, which runs AFTER the model
            // stage, so it is still 0 here and the guard always fails.
            // The oracle sets Setpri at editor time, before arima. The
            // post-model `Adj -> Sprior` copy in x11int currently covers
            // for it, and does so correctly whenever Adj == Sprior at
            // that point -- true for every gated spec, and NOT true on
            // the Picktd-flip corner walled in automx.cpp, where it is
            // the whole cause. Do not delete this block: it becomes live
            // the moment Setpri moves ahead of the model stage.
            if (aj.setpri >= 1)
                copy(aj.adj.data(), aj.nadj, -1, &ip.sprior(aj.setpri));
            if ((pu.nustad == 0 || pu.nuspad == 0) && pr.kfmt > 0) pr.kfmt = 0;
        }
        if (tdmdl1 > 0) tdmdl1 = 1;
    } else {
        if (!pktd && pk.picktd) {
            // tdaic.f:603/611 -- Sprior takes the prior the Picktd
            // transition just put in Adj. DEAD IN THIS PORT: setpri is
            // assigned only in x11_prestage, which runs AFTER the model
            // stage, so it is still 0 here and the guard always fails.
            // The oracle sets Setpri at editor time, before arima. The
            // post-model `Adj -> Sprior` copy in x11int currently covers
            // for it, and does so correctly whenever Adj == Sprior at
            // that point -- true for every gated spec, and NOT true on
            // the Picktd-flip corner walled in automx.cpp, where it is
            // the whole cause. Do not delete this block: it becomes live
            // the moment Setpri moves ahead of the model stage.
            if (aj.setpri >= 1)
                copy(aj.adj.data(), aj.nadj, -1, &ip.sprior(aj.setpri));
            if (pr.kfmt == 0) pr.kfmt = 1;
            if (pu.nuspad == 0 || pu.npser == 0) {
                pu.prmser.assign("LPY");
                pu.npser = 3;
            }
        }
        if (ar.ntdvec == 2)
            tdmdl1 = 0;
        else if (ar.aicint == ar.tdayvc(ar.ntdvec))
            tdmdl1 = 2;
        else
            tdmdl1 = 0;
    }
}

// ---------------------------------------------------------------------------
// easaic.f
// ---------------------------------------------------------------------------
void easaic(X13Context& ctx, double* trnsrs, double* a, int& nefobs, int& na,
            int& frstry, bool& lester, bool lsumm) {
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& ar = ctx.arima;
    auto& pr = ctx.prior;
    auto& aj = ctx.adj;
    auto& ext = ctx.extend;
    auto& x11 = ctx.x11adj;

    auto find_eas_group = [&]() {
        int g = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                       "Easter");
        if (g == 0)
            g = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                       "StatCanEaster");
        if (g == 0)
            g = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                       "StockEaster");
        return g;
    };

    double aiceas = DNOTST;
    double aicno = DNOTST;
    bool lmanyE = false;
    // easaic.f:68-75 -- the savelog header. `testalleaster` is NOT under the
    // `aictest.` prefix but belongs to this block, and Easvec(Neasvc)==99 is
    // the "test every window at once" sentinel.
    if (lsumm) {
        auto& sv = ctx.aictest_log;
        sv.ran = true;
        sv.easter_aicc.clear();
        sv.testalleaster = (ar.easvec(ar.neasvc) == 99);
        sv.have_testalleaster = true;
        sv.easter_num = ar.neasvc - 1;
    }
    int nbnoe = 0, nbe = 0;
    int easgrp = 0;

    for (int i = 1; i <= ar.neasvc; ++i) {
        // Delete any Easter regressors already present (easaic.f:80-98).
        if (x11.neas > 0) {
            for (int j = 1; j <= x11.neas; ++j) {
                easgrp = find_eas_group();
                if (easgrp > 0) {
                    int begcol = m.grp(easgrp - 1);
                    int ncol = m.grp(easgrp) - begcol;
                    dlrgef(ctx, begcol, ar.nrxy, ncol);
                    if (ctx.error.lfatal) return;
                }
            }
            x11.neas = 0;
        }

        // Add the new Easter regressor (easaic.f:102-130).
        if (i > 1 || easgrp > 0) {
            lmanyE = (i == ar.neasvc && ar.easvec(ar.neasvc) == 99);
            if (lmanyE) {
                for (int j = 2; j <= ar.neasvc - 1; ++j) {
                    addeas(ctx, ar.easvec(j) + m.easidx, m.easidx, ar.eastst);
                    if (ctx.error.lfatal) return;
                }
                x11.neas = ar.neasvc - 2;
            } else if (i > 1) {
                addeas(ctx, ar.easvec(i) + m.easidx, m.easidx, ar.eastst);
                if (ctx.error.lfatal) return;
                x11.neas = 1;
            }
            regvar(ctx, trnsrs, ext.nobspf, ar.fctdrp, ext.nfcst, 0,
                   ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, pr.priadj,
                   ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
            if (ctx.error.lfatal) return;
        }

        if (m.nopr > 0) {
            int endlag = m.opr(m.nopr) - 1;
            for (int ilag = 1; ilag <= endlag; ++ilag)
                if (!m.arimaf(ilag)) d.arimap(ilag) = m.ap1(ilag);
        }

        bool argok = ar.lautom || ar.lautox;
        rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs, argok);
        if (!ctx.error.lfatal && (ar.lautom || ar.lautox) && !argok) abend(ctx);
        if (ctx.error.lfatal) return;
        if (armaer_is_fatal(d.armaer) ||
            ((ar.lautom || ar.lautox) && !argok)) {
            lester = true;
            return;
        }
        if (d.armaer != 0) d.armaer = 0;

        prlkhd(ctx, &ar.y(ar.frstsy), &aj.adj(aj.adj1st), aj.adjmod, ar.fcntyp,
               ar.lam);
        if (ctx.error.lfatal) return;
        double aicc = ctx.lkhd.aicc;

        // easaic.f:188-196. Three label forms, not one: `noeaster` for i==1,
        // `alleaster` for the 99 sentinel, else `easter` + the window written
        // `i2.2` (format 1060), which is what makes the keys easter01/08/15.
        if (lsumm) {
            std::string lab;
            if (i == 1) {
                lab = "noeaster";
            } else if (lmanyE) {
                lab = "alleaster";
            } else {
                int w = ar.easvec(i);
                lab = "easter" + std::string(w < 10 ? "0" : "") + std::to_string(w);
            }
            ctx.aictest_log.easter_aicc.push_back({lab, aicc});
        }

        if (i == 1) {
            aicno = aicc;
            nbnoe = m.nb;
        } else {
            if (i == 2) {
                aiceas = aicc;
                ar.aicind = ar.easvec(i);
                nbe = m.nb;
            } else if (lmanyE) {
                if (!dpeq(ar.pvaic, DNOTST)) {
                    int aicdf = m.nb - nbe;
                    double thiscv = chsppf(ar.pvaic, aicdf);
                    ar.rgaicd(PEAIC) = thiscv - 2.0 * static_cast<double>(aicdf);
                }
            }
            ar.dfaice = aiceas - aicc;
            if (ar.dfaice > ar.rgaicd(PEAIC)) {
                ar.aicind = ar.easvec(i);
                aiceas = aicc;
                if (!dpeq(ar.pvaic, DNOTST)) nbe = m.nb;
            }
        }
    }

    // ---- decide Easter vs no-Easter (easaic.f:220-231) ----
    ar.dfaice = aicno - aiceas;
    if (!dpeq(ar.pvaic, DNOTST)) {
        int aicdf = nbe - nbnoe;
        double thiscv = chsppf(ar.pvaic, aicdf);
        (void)thiscv;
        ar.rgaicd(PEAIC) = thiscv - 2.0;  // easaic.f:224 (note: NOT thiscv-2*df)
    }
    if (ar.dfaice > ar.rgaicd(PEAIC)) {
        // keep the best Easter model
    } else {
        ar.aicind = -1;
    }

    // ---- re-estimate the best model if it was not the last one (easaic.f:295) ----
    if (ar.aicind < ar.easvec(ar.neasvc)) {
        for (int j = 1; j <= x11.neas; ++j) {
            easgrp = find_eas_group();
            int begcol = m.grp(easgrp - 1);
            int ncol = m.grp(easgrp) - begcol;
            dlrgef(ctx, begcol, ar.nrxy, ncol);
            if (ctx.error.lfatal) return;
        }
        if (!ctx.error.lfatal && ar.aicind >= 0)
            addeas(ctx, ar.aicind + m.easidx, m.easidx, ar.eastst);
        if (m.nopr > 0) {
            int endlag = m.opr(m.nopr) - 1;
            for (int ilag = 1; ilag <= endlag; ++ilag)
                if (!m.arimaf(ilag)) d.arimap(ilag) = m.ap1(ilag);
        }
        bool argok = ar.lautom || ar.lautox;
        if (!ctx.error.lfatal)
            regvar(ctx, trnsrs, ext.nobspf, ar.fctdrp, ext.nfcst, 0,
                   ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, pr.priadj,
                   ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
        if (!ctx.error.lfatal)
            rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs, argok);
        if (!ctx.error.lfatal && (ar.lautom || ar.lautox) && !argok) lester = true;
    }
}

// ---------------------------------------------------------------------------
// addlom.f -- add a lom/loq/lpyear regressor group (possibly change-of-regime).
void addlom(X13Context& ctx, const int* aicrgm, int aicln0, int sp, int lnindx) {
    using namespace prm;
    std::string datstr;
    if (aicrgm[0] != NOTSET) {
        datstr = wrtdat(aicrgm, sp);
        if (ctx.error.lfatal) return;
    }
    if (lnindx == 0) return;
    std::string base;
    int varln, varln1, varln2;
    if (lnindx == 1) { base = "Length-of-Month";   varln = PRGTLM; varln1 = PRRTLM; varln2 = PRATLM; }
    else if (lnindx == 2) { base = "Length-of-Quarter"; varln = PRGTLQ; varln1 = PRRTLQ; varln2 = PRATLQ; }
    else { base = "Leap Year"; varln = PRGTLY; varln1 = PRRTLY; varln2 = PRATLY; }
    if (aicln0 == 0) {
        std::string gt = base;
        if (aicrgm[0] != NOTSET) gt = base + " (after " + datstr + ")";
        adrgef(ctx, DNOTST, base, gt, varln, false, false);
        if (ctx.error.lfatal) return;
    }
    if (aicrgm[0] != NOTSET) {
        if (aicln0 >= 0) {
            std::string gt = (aicln0 == 0)
                ? base + " (change for before " + datstr + ")"
                : base + " (before " + datstr + ")";
            adrgef(ctx, DNOTST, base + " I", gt, varln1, false, false);
        } else {
            std::string gt = base + " (starting " + datstr + ")";
            adrgef(ctx, DNOTST, base + " II", gt, varln2, false, false);
        }
        if (ctx.error.lfatal) return;
    }
}

// find the lom/loq/lpyear GROUP by title (lomaic.f:49-55).
static int find_lom_group(model_cmn& m, int lomtst) {
    const char* t = (lomtst == 1) ? "Length-of-Month"
                  : (lomtst == 2) ? "Length-of-Quarter" : "Leap Year";
    return strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl, t);
}

// remove every Length-of-* / Leap Year column (lomaic.f:124-133 / 236-247).
static void remove_lom_cols(X13Context& ctx, int nrxy) {
    auto& m = ctx.model;
    while (true) {
        int ilom = strinx(true, m.colttl.raw(), m.colptr.data(), 1, m.ncoltl,
                          "Length-of-");
        if (ilom == 0)
            ilom = strinx(true, m.colttl.raw(), m.colptr.data(), 1, m.ncoltl,
                          "Leap Year");
        if (ilom <= 0) break;
        dlrgef(ctx, ilom, nrxy, 1);
        if (ctx.error.lfatal) return;
    }
}

// lomaic.f -- length-of-month/-quarter/leap-year AIC test. Estimates the model
// with and without the regressor, keeps the lower-AICC form (Rgaicd(PLAIC) gap).
void lomaic(X13Context& ctx, double* trnsrs, double* a, int& nefobs, int& na,
            int& frstry, bool& lester, bool lsumm) {
    using namespace prm;
    auto& m = ctx.model; auto& d = ctx.mdldat; auto& ar = ctx.arima;
    auto& pr = ctx.prior; auto& aj = ctx.adj; auto& ext = ctx.extend;

    bool argok = ar.lautom || ar.lautox;
    auto reest = [&]() {
        regvar(ctx, trnsrs, ext.nobspf, ar.fctdrp, ext.nfcst, 0, ar.userx.data(),
               ar.bgusrx.data(), ar.nrusrx, pr.priadj, ar.reglom, ar.nrxy,
               ar.begxy.data(), frstry, true, ar.elong);
        if (ctx.error.lfatal) return;
        argok = ar.lautom || ar.lautox;
        rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs, argok);
        if (!ctx.error.lfatal && (ar.lautom || ar.lautox) && !argok) abend(ctx);
    };
    auto est_err = [&]() {
        int e = d.armaer;
        return e == PMXIER || e == PSNGER || e == PISNER || e == PNIFER ||
               e == PNIMER || e == PCNTER || e == POBFN0 || e < 0 ||
               ((ar.lautom || ar.lautox) && !argok);
    };

    int klm = find_lom_group(m, ar.lomtst);
    bool lreest = false;

    // Estimate the model as given, take its AICC.
    reest();
    if (ctx.error.lfatal) return;
    if (est_err()) { lester = true; return; }
    if (d.armaer != 0) d.armaer = 0;
    prlkhd(ctx, &ar.y(ar.frstsy), &aj.adj(aj.adj1st), aj.adjmod, ar.fcntyp, ar.lam);
    if (ctx.error.lfatal) return;
    double aiclom = DNOTST, aicnol = DNOTST;
    // lomaic.f:96-105 -- format 1012 keys BOTH halves off the same stem:
    // `aictest.<stem>.aicc.<stem>` with the regressor in, `...aicc.no<stem>`
    // without. The stem is mklnlb's abbreviation (lom / loq / lpyear).
    std::string lom_abbr;
    if (lsumm) {
        auto& sv = ctx.aictest_log;
        sv.ran = true;
        sv.lom_aicc.clear();
        mklnlb(ctx, ar.lomtst, ctx.picktd.lndate.data(), ctx.picktd.lnzero,
               m.sp, lom_abbr);
        if (ctx.error.lfatal) return;
        sv.lom_abbrev = lom_abbr;
        sv.lom_aicc.push_back({(klm > 0 ? lom_abbr : "no" + lom_abbr),
                               ctx.lkhd.aicc});
    }
    if (klm > 0) aiclom = ctx.lkhd.aicc; else aicnol = ctx.lkhd.aicc;

    // Toggle the regressor: add it if absent, else remove it.
    if (klm == 0) {
        int aicrgm[2] = {NOTSET, NOTSET};
        addlom(ctx, aicrgm, 0, m.sp, ar.lomtst);
        if (ctx.error.lfatal) return;
        klm = find_lom_group(m, ar.lomtst);
    } else {
        remove_lom_cols(ctx, ar.nrxy);
        if (ctx.error.lfatal) return;
        klm = 0;
    }

    // Re-estimate and take the other AICC.
    reest();
    if (ctx.error.lfatal) return;
    if (est_err()) { lester = true; return; }
    if (d.armaer != 0) d.armaer = 0;
    prlkhd(ctx, &ar.y(ar.frstsy), &aj.adj(aj.adj1st), aj.adjmod, ar.fcntyp, ar.lam);
    if (ctx.error.lfatal) return;
    // lomaic.f:171-180 -- the second fit, with the regressor toggled, so the
    // label flips too.
    if (lsumm)
        ctx.aictest_log.lom_aicc.push_back(
            {(klm > 0 ? lom_abbr : "no" + lom_abbr), ctx.lkhd.aicc});
    if (klm > 0) aiclom = ctx.lkhd.aicc; else aicnol = ctx.lkhd.aicc;

    // Keep whichever AICC is lower (lomaic.f:186-263). Pvaic path deferred.
    ar.dfaicl = aicnol - aiclom;
    if (ar.dfaicl > ar.rgaicd(PLAIC)) {
        if (klm == 0) {   // prefer WITH lom but it is currently removed
            restor_model(ctx);
            regvar(ctx, trnsrs, ext.nobspf, ar.fctdrp, ext.nfcst, 0,
                   ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, pr.priadj,
                   ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
            if (ctx.error.lfatal) return;
            lreest = true;
        }
    } else {
        if (klm > 0) {    // prefer WITHOUT lom but it is currently present
            remove_lom_cols(ctx, ar.nrxy);
            if (ctx.error.lfatal) return;
            lreest = true;
        }
    }

    if (lreest) {
        regvar(ctx, trnsrs, ext.nobspf, ar.fctdrp, ext.nfcst, 0, ar.userx.data(),
               ar.bgusrx.data(), ar.nrusrx, pr.priadj, ar.reglom, ar.nrxy,
               ar.begxy.data(), frstry, true, ar.elong);
        if (!ctx.error.lfatal)
            rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs, argok);
        if (!ctx.error.lfatal && (ar.lautom || ar.lautox) && !argok) lester = true;
    }
}

// editor.f:1151-1166 -- build the TD candidate vector from Itdtst. In the oracle
// this runs ONCE, in the editor, before any model is estimated; this port has no
// editor block for it, so each caller of the AIC tests runs it at its own
// equivalent point (arima.f's explicit path just before tdaic, automx's once
// before the candidate loop -- tdaic itself adds and deletes TD groups, so a
// per-candidate rebuild would read a design the editor never saw).
// ktd/kstd flag an already-present flow/stock TD group.
void aictest_td_vectors(X13Context& ctx) {
    auto& m = ctx.model;
    auto& ar = ctx.arima;
    const int ktd = find_td_group(m) > 0 ? 1 : 0;
    const int kstd = 0;
    ar.ntdvec = 2;
    ar.tdayvc(1) = 0;
    ar.tdayvc(2) = ar.itdtst;
    if ((ar.itdtst <= 2 && ktd == 0) || (ar.itdtst == 3 && kstd == 0)) {
        ar.tdayvc(3) = ar.itdtst + 3;
        ar.ntdvec = 3;
    }
    if (m.isrflw == 2 && ktd == 0 && kstd == 0 && ar.itdtst <= 2) {
        ar.tdayvc(2) = 3;
        ar.tdayvc(3) = 6;
        ar.itdtst = 3;
    }
}

// editor.f:1410-1442 -- the aictest=easter candidate windows Easvec=(-1,1,8,15).
void aictest_eas_vectors(X13Context& ctx) {
    auto& ar = ctx.arima;
    if (ar.eastst == 0) ar.eastst = 1;
    ar.neasvc = 4;
    ar.easvec(1) = -1;
    ar.easvec(2) = 1;
    ar.easvec(3) = 8;
    ar.easvec(4) = 15;
    ctx.x11adj.neas = 0;
    if (!ctx.x11adj.finhol) ctx.x11adj.finhol = true;
}

// arima.f:569-700 -- explicit-model AIC regressor test. Runs the td/lom/easter
// AIC tests (user/chi deferred) in place of the plain rgarma when an explicit
// arima{} model carries aictest=(...). Setup mirrors automd's block-1.
void explicit_aictest(X13Context& ctx, double* trnsrs, double* a, int& nefobs,
                      int& na, int& frstry) {
    using namespace prm;
    auto& m = ctx.model; auto& ar = ctx.arima;

    // Pvaic / Rgaicd / Traicd are gtinpt defaults (gtinpt.f:293-300) and are
    // set there; resetting them here discarded a `regression{aicdiff=}` or
    // `pvaictest=` the spec supplied. Measured on
    // `generated/airline_aictest-td-aicdiff`: with aicdiff=(20.0) the oracle
    // REJECTS trading day (nreg 0) and the engine kept it (nreg 1).

    // Prior-adjustment span the AIC tests' leap-year preadjust needs
    // (automd.cpp:60-71). Nbcst==0 for these specs.
    const int nbcst = ctx.extend.nbcst < 0 ? 0 : ctx.extend.nbcst;
    addate(ctx.mdldat.begspn.data(), m.sp, -nbcst, ctx.adj.begadj.data());
    const int nfc = ctx.extend.nfcst < 0 ? 0 : ctx.extend.nfcst;
    const int tail = m.sp > (nfc - ar.fctdrp) ? m.sp : (nfc - ar.fctdrp);
    ctx.adj.nadj = ctx.mdldat.nspobs + nbcst + tail;
    int a1st = 0;
    dfdate(ctx.mdldat.begspn.data(), ctx.adj.begadj.data(), m.sp, a1st);
    ctx.adj.adj1st = a1st + 1;

    // Back up the initial ARMA coefficients into Ap1 (editor.f:921-925). tdaic
    // seeds each candidate's arimap from Ap1; leaving it 0 (not DNOTST) would
    // suppress rgarma's PNT1 default and start the candidates from ARMA=0,
    // perturbing the optimizer trajectory (niter/nfev).
    if (m.nopr > 0) {
        int endlag = m.opr(m.nopr) - 1;
        for (int ilag = 1; ilag <= endlag; ++ilag)
            if (!m.arimaf(ilag)) m.ap1(ilag) = ctx.mdldat.arimap(ilag);
    }

    // Save the entry model so lomaic's restor branch has a valid target.
    ssprep_save(ctx);

    // usraic (user-regressor AIC) and chkchi (chi-square holiday) are not
    // ported; fail loudly rather than replace rgarma with a no-op (arima.f:644-
    // 692). The tested td/lom/easter contract is unaffected.
    if ((ar.luser && ctx.usrreg.ncusrx > 0) ||
        (ar.ch2tst && ctx.usrreg.nguhl > 0)) {
        abend(ctx);
        return;
    }

    bool lester = false;
    if (ar.itdtst > 0) {
        aictest_td_vectors(ctx);
        int tdmdl1 = 0;
        tdaic(ctx, trnsrs, a, nefobs, na, frstry, tdmdl1, false, lester,
              /*lsumm=*/true);
        if (ctx.error.lfatal) return;
        ssprep_save(ctx);   // arima.f:599
    }
    if (!lester && ar.lomtst > 0) {
        lomaic(ctx, trnsrs, a, nefobs, na, frstry, lester, /*lsumm=*/true);
        if (ctx.error.lfatal) return;
        ssprep_save(ctx);   // arima.f:619
    }
    if (!lester && ar.leastr) {
        aictest_eas_vectors(ctx);
        easaic(ctx, trnsrs, a, nefobs, na, frstry, lester, /*lsumm=*/true);
        if (ctx.error.lfatal) return;
    }
    // usraic (user) + chkchi (chi-square holiday) deferred.

    // arima.f:697-700: turn off the AIC-test options for the rest of the run.
    ar.itdtst = 0;
    ar.leastr = false;
    ar.luser = false;
    ar.ch2tst = false;
}

}  // namespace x13
