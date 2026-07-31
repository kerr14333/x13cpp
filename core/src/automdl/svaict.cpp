// svaict.cpp -- svaict.f + its three label builders (mktdlb.f / mklnlb.f /
// mkealb.f). See svaict.hpp for scope and for what is deliberately not here.

#include "automdl/svaict.hpp"

#include "automdl/aictst.hpp"       // strinx
#include "common/x13context.hpp"
#include "gen/model.hpp"            // PTDAIC / PLAIC / PEAIC / PUAIC
#include "gen/notset.hpp"           // prm::NOTSET
#include "regarima/outlier.hpp"     // wrtdat
#include "specparse/specparse.hpp"  // itoc

namespace x13 {

namespace {

// The change-of-regime suffix shared verbatim by mktdlb.f:39-56 and
// mklnlb.f:38-54: the number of slashes on each side encodes the zero-fill
// mode, which is why this is four cases rather than one.
std::string regime_suffix(X13Context& ctx, const int* aicrgm, int zero, int sp) {
    if (aicrgm == nullptr || aicrgm[0] == prm::NOTSET) return std::string();
    std::string datstr = wrtdat(aicrgm, sp);
    if (ctx.error.lfatal) return std::string();
    switch (zero) {
    case 0:  return "/" + datstr + "/";
    case 1:  return "/" + datstr + "//";
    case 2:  return "//" + datstr + "//";
    default: return "//" + datstr + "/";
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// mktdlb.f
// ---------------------------------------------------------------------------
std::string mktdlb(X13Context& ctx, int itdtst, int aicstk, const int* aicrgm,
                   int tdzero, int sp) {
    std::string s;
    if (itdtst == 1) {
        s = "td";
    } else if (itdtst == 2) {
        s = "tdnolpyear";
    } else if (itdtst == 4) {
        s = "td1coef";
    } else if (itdtst == 5) {
        s = "td1nolpyear";
    } else {
        // :28-38 -- the stock forms carry the stock day in brackets. The
        // Fortran writes the '[' as part of the stem, then itoc's the number
        // and closes it.
        s = (itdtst == 6) ? "tdstock1coef[" : "tdstock[";
        // itoc writes INTO the string at a 1-based position and abends when the
        // buffer is too short -- it is transcribed from a Fortran CHARACTER*(N)
        // and does not grow. mktdlb.f's own buffer is 30 blanks; pad to that,
        // then cut back to what itoc actually wrote.
        const std::size_t stem = s.size();
        s.resize(30, ' ');
        int pos = static_cast<int>(stem) + 1;
        itoc(ctx, aicstk, s, pos);
        if (ctx.error.lfatal) return std::string();
        s.resize(static_cast<std::size_t>(pos - 1));
        s += "]";
    }
    s += regime_suffix(ctx, aicrgm, tdzero, sp);
    return s;
}

// ---------------------------------------------------------------------------
// mklnlb.f
// ---------------------------------------------------------------------------
std::string mklnlb(X13Context& ctx, int lomtst, const int* aicrgm, int lnzero,
                   int sp, std::string& abbrev) {
    // :22-31. Note the Fortran leaves Lnstr blank (and Nlnchr UNSET) for any
    // Lomtst outside 1..3; reproduced as an empty label rather than a guess.
    if (lomtst == 1) {
        abbrev = "lom";
    } else if (lomtst == 2) {
        abbrev = "loq";
    } else if (lomtst == 3) {
        abbrev = "lpyear";
    } else {
        abbrev.clear();
        return std::string();
    }
    return abbrev + regime_suffix(ctx, aicrgm, lnzero, sp);
}

// ---------------------------------------------------------------------------
// mkealb.f
// ---------------------------------------------------------------------------
std::string mkealb(X13Context& ctx, int eastst, int easidx, int easwin,
                   bool lbase) {
    std::string stem;
    if (easidx == 0)
        stem = (eastst == 1) ? "easter[" : "easterstock[";
    else
        stem = "statcaneaster[";

    // :24-28 -- itoc into a 2-char buffer starting at position 1, then
    // `eastr(N+1:N+nwin) = cwin(1:nwin-1)//']'`. nwin ends one PAST the digits,
    // so that expression is exactly "<digits>]".
    std::string cwin(2, ' ');   // mkealb.f's `CHARACTER cwin*2`; see mktdlb above
    int nwin = 1;
    itoc(ctx, easwin, cwin, nwin);
    if (ctx.error.lfatal) return std::string();
    std::string full = stem + cwin.substr(0, static_cast<std::size_t>(nwin - 1)) + "]";

    // :30-34, and this is the quirk worth naming. With Lbase the Fortran does
    // `Neachr = Neachr - 1`, where Neachr is the length of the stem INCLUDING
    // its '['. So the caller gets the stem with the bracket chopped off --
    // `easter`, not `easter[` and not `easter[8]`. That is what
    // `aictest.easter.reg: easter` in the goldens is; it is not a separate
    // label, just this subtraction.
    if (lbase) return stem.substr(0, stem.size() - 1);
    return full;
}

// ---------------------------------------------------------------------------
// svaict.f
// ---------------------------------------------------------------------------
void svaict(X13Context& ctx, bool savtd, bool savlom, bool saveas, bool savusr,
            bool hvmdl) {
    auto& m = ctx.model;
    auto& ar = ctx.arima;
    auto& out = ctx.aictest_log;
    out.ran = true;

    auto group = [&](const char* const* titles, int ntitles) -> int {
        for (int i = 0; i < ntitles; ++i) {
            int g = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                           titles[i]);
            if (g > 0) return g;
        }
        return 0;
    };

    // ---- trading day (:26-54) ----
    if (savtd) {
        auto& g = out.td;
        g.tested = true;
        g.nomodel = !hvmdl;
        if (hvmdl) {
            static const char* kTd[4] = {"Trading Day", "1-Coefficient Trading Day",
                                         "Stock Trading Day",
                                         "1-Coefficient Stock Trading Day"};
            g.accepted = group(kTd, 4) > 0;
            if (g.accepted) {
                g.label = mktdlb(ctx, ar.aicint, ar.aicstk,
                                 ctx.picktd.tddate.data(), ctx.picktd.tdzero,
                                 ctx.model.sp);
                if (ctx.error.lfatal) return;
            }
            g.diff = ar.dfaict;
            g.have_cvaic = ar.rgaicd(prm::PTDAIC) > 0.0;
            g.cvaic = ar.rgaicd(prm::PTDAIC);
        }
    }

    // ---- length of month / quarter / leap year (:56-89) ----
    if (savlom) {
        auto& g = out.lom;
        g.tested = true;
        g.nomodel = !hvmdl;
        // NOT inside the Hvmdl guard in the Fortran: mklnlb runs first and
        // `aictest.<abbr>.reg` is written unconditionally (:57-61).
        g.label = mklnlb(ctx, ar.lomtst, ctx.picktd.lndate.data(),
                         ctx.picktd.lnzero, ctx.model.sp, out.lom_abbrev);
        if (ctx.error.lfatal) return;
        if (hvmdl) {
            static const char* kLom[3] = {"Length-of-Month", "Length-of-Quarter",
                                          "Leap Year"};
            g.accepted = group(kLom, 3) > 0;
            g.diff = ar.dfaicl;
            // CB-candidate, transcribed as written: the cvaic line for THIS
            // group is gated on `Rgaicd(PTDAIC)` -- the TRADING-DAY threshold
            // -- while the value it writes is `Rgaicd(PLAIC)` (svaict.f:79-81).
            // Every other group gates on its own index. Not exercised by the
            // corpus (no golden carries aictest.cvaic.lom), so it is left as
            // the Fortran has it rather than "fixed".
            g.have_cvaic = ar.rgaicd(prm::PTDAIC) > 0.0;
            g.cvaic = ar.rgaicd(prm::PLAIC);
        }
    }

    // ---- Easter (:91-150) ----
    if (saveas) {
        auto& g = out.easter;
        g.tested = true;
        g.nomodel = !hvmdl;
        // Also outside the Hvmdl guard (:92-95).
        g.label = mkealb(ctx, ar.eastst, m.easidx, ar.aicind, /*lbase=*/true);
        if (ctx.error.lfatal) return;
        out.easter_window.clear();
        if (hvmdl) {
            static const char* kEas[3] = {"Easter", "StatCanEaster", "StockEaster"};
            g.accepted = group(kEas, 3) > 0;
            if (g.accepted) {
                if (ar.aicind == 99) {
                    // :123-125 -- the multi-window form prints Easvec(2..Neasvc-1),
                    // format 1025 (5i6).
                    for (int j = 2; j <= ar.neasvc - 1; ++j)
                        out.easter_window.push_back(ar.easvec(j));
                } else {
                    out.easter_window.push_back(ar.aicind);
                }
            } else {
                out.easter_window.push_back(-99999);
            }
            g.diff = ar.dfaice;
            g.have_cvaic = ar.rgaicd(prm::PEAIC) > 0.0;
            g.cvaic = ar.rgaicd(prm::PEAIC);
        } else {
            out.easter_window.push_back(-99999);
        }
    }

    // ---- user-defined (:152-176) ----
    if (savusr) {
        auto& g = out.user;
        g.tested = true;
        g.nomodel = !hvmdl;
        if (hvmdl) {
            static const char* kUsr[1] = {"User-defined"};
            g.accepted = group(kUsr, 1) > 0;
            g.diff = ar.dfaicu;
            g.have_cvaic = ar.rgaicd(prm::PUAIC) > 0.0;
            g.cvaic = ar.rgaicd(prm::PUAIC);
        }
    }
}

}  // namespace x13
