// svaict.hpp -- svaict.f: the `aictest.*` savelog block.
//
// After the AIC regressor tests have run (tdaic / lomaic / easaic / usraic),
// the oracle reports which group each test KEPT, the AICC difference that
// decided it, and -- when a non-default threshold was in force -- the
// threshold itself. arima.f calls it five times: once for the whole set after
// the model is selected (:465, Mdltxt "selected ") and once per group inside
// the explicit-aictest block (:583/:607/:630/:649, "estimated"). Mdltxt reaches
// only the `.log` file, never the `.udg`, so every call writes the same key
// set; no corpus golden carries a duplicated `aictest.*` key, i.e. exactly one
// of those calls reaches Nform on any given run.
//
// NO FILE OUTPUT. The oracle writes these lines to Nform (the `.udg`) and Ng
// (the `.log`) directly; this port fills X13Context::aictest_log and leaves
// rendering to the caller, per the project's caller-driven-output rule. The
// `.log` half (`   AICtd : ...`, format 1030/1050/1055) is not modelled: no
// gate reads the `.log`, and the values behind it are the same ones below.
//
// DEFERRED, with the reason: `aictest.pv` (arima.f:463, `1 - Pvaic`) is not
// part of svaict at all -- it is written by its caller, and only when
// `regression{pvaictest=}` was given. No corpus spec sets it, so it has no
// golden to gate against; it belongs with the caller if one ever does.
#ifndef X13_AUTOMDL_SVAICT_HPP
#define X13_AUTOMDL_SVAICT_HPP

#include <string>
#include <vector>

namespace x13 {

struct X13Context;

// mktdlb.f -- the trading-day label: `td`, `tdnolpyear`, `td1coef`,
// `td1nolpyear`, `tdstock[N]`, `tdstock1coef[N]`, with an optional
// `/date/`-style change-of-regime suffix whose slashes encode Tdzero.
std::string mktdlb(X13Context& ctx, int itdtst, int aicstk, const int* aicrgm,
                   int tdzero, int sp);

// mklnlb.f -- the length-of-month family label (`lom` / `loq` / `lpyear`) plus
// the same change-of-regime suffix. `abbrev` is the bare stem, which is what
// the key names are built from (`aictest.lom`, `aictest.diff.lom`).
std::string mklnlb(X13Context& ctx, int lomtst, const int* aicrgm, int lnzero,
                   int sp, std::string& abbrev);

// mkealb.f -- the Easter label. `lbase` selects the STEM only (`easter`,
// `easterstock`, `statcaneaster`); with lbase false the window and its closing
// bracket are included (`easter[8]`). Reproduces the Fortran's own length
// arithmetic, which drops the OPENING bracket in the lbase case -- see the
// note in the .cpp.
std::string mkealb(X13Context& ctx, int eastst, int easidx, int easwin,
                   bool lbase);

// The values svaict reports, in the order the oracle writes them. Rendering
// (and the exact Fortran FORMATs) belongs to the caller; see tools/x13run_m3.
struct AictestSavelog {
    bool ran = false;

    struct Group {
        bool tested = false;    // the Sav* flag -- was this test requested
        bool nomodel = false;   // .not.Hvmdl -> the value is the string "nomodel"
        bool accepted = false;  // strinx found the group in the final model
        std::string label;      // mktdlb / mklnlb / mkealb result ("" when n/a)
        double diff = 0.0;      // Dfaict / Dfaicl / Dfaice / Dfaicu
        bool have_cvaic = false;
        double cvaic = 0.0;     // Rgaicd(...), written only when > 0
    };

    // The per-candidate AICC tables the AIC TESTS themselves write -- tdaic.f
    // :99-103 and :393/:399, easaic.f and lomaic.f likewise. They are a
    // different block from svaict's verdicts above and are emitted ONLY on the
    // explicit-aictest path: every tdaic/easaic/lomaic call outside arima.f
    // passes `Lsumm = 0` (automd.f x3 sites, automx.f x2), which is why a
    // pickmdl or automdl golden carries `aictest.td` but never `aictest.td.num`.
    struct AiccRow {
        std::string label;   // "notd" / "td" / "td1coef" / "noeaster" / ...
        double aicc = 0.0;
    };
    std::vector<AiccRow> td_aicc;
    int td_num = -1;                // Ntdvec-1; -1 == the block did not run
    std::string td_reg, td_reg2;    // aictest.td.reg / .reg2 (reg2 iff Ntdvec==3)
    std::vector<AiccRow> easter_aicc;
    int easter_num = -1;
    // easaic.f:69-73's `testalleaster: yes|no`. Not under the `aictest.`
    // prefix but part of the same block, so it is carried here rather than
    // left as an unowned key nothing emits.
    bool have_testalleaster = false;
    bool testalleaster = false;
    std::vector<AiccRow> lom_aicc;
    // usraic.f 1012: ('aictest.u.aicc.',a,': ',e29.15) -- 'user' then
    // 'nouser', in the order the two models are fitted.
    std::vector<AiccRow> user_aicc;

    Group td;
    Group lom;                     // key stem is lom.abbrev, not "lom"
    std::string lom_abbrev;        // "lom" / "loq" / "lpyear"
    Group easter;
    // aictest.e.window: Aicind, or Easvec(2..Neasvc-1) when Aicind == 99.
    // -99999 is the oracle's own literal for the rejected / no-model cases.
    std::vector<int> easter_window;
    Group user;
};

// svaict.f. The four flags are the oracle's Savtd/Savlom/Saveas/Savusr, i.e.
// `Itdtst.gt.0` / `Lomtst.gt.0` / `Leastr` / `Luser` at the call site; hvmdl is
// its Hvmdl. Appends nothing and overwrites ctx.aictest_log for the groups
// named, matching the oracle's one-write-per-key behaviour.
void svaict(X13Context& ctx, bool savtd, bool savlom, bool saveas, bool savusr,
            bool hvmdl);

}  // namespace x13

#endif  // X13_AUTOMDL_SVAICT_HPP
