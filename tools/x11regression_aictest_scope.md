# x11regression{} aictest + irregular-outlier sub-engine — port plan

**STATUS: CLOSED (bit-exact).** Both increments landed: Easter AICC window
selection (1) + idotlr AO detection with 14-col xrm and b16/c16 (2). Gated in
`tests/parity/test_x11regression_tables.py`.

**Goal:** close `airline_x11regression-aictest` (`variables=(td) aictest=(easter)`)
bit-exact. Sibling of the just-closed OLS prior-TD (`airline_x11regression-td`,
Ixreg>=2/xrgdrv); same transparent-pass hoist, but x11mdl now runs its
holiday-AIC + automatic-outlier branches, not just the TD-only path.

Ground truth: `oracle/fortran/x11mdl.f` (890) + `x11aic.f` (604) + `xrlkhd.f`
(56) + `addeas.f` (45). Golden xrm is **14 cols**: 6 TD + Easter[15] + 7 auto-AO
(AO1950.May, AO1950.Nov, AO1951.May, AO1952.Jun, AO1953.Apr, AO1954.Feb,
AO1960.Mar). Golden .udg canaries (the intermediate gate):
```
aictest.xe.aicc.noeaster: -748.654   easter01: -750.843
aictest.xe.aicc.easter08: -750.117   easter15: -751.021  (lowest -> won)
aictest.xe: yes   aictest.xe.window: 15
```

## Flow (x11mdl.f, Kpart==2 B-iteration then Kpart==3 C-iteration)
1. **addeas/addtd first-iteration seed** (x11mdl:140-176, Kpart==2): if Xeastr and
   no Easter group yet, `addeas(Xeasvc(3)+Easidx, Easidx, 1)` seeds an easter
   column; likewise addtd for Xtdtst. Sets Axrghl/Axrgtd.
2. **store irregular -> trnsrs**, xrgtrn transform, **tdxtrm** extreme exclusion
   (all ported for the TD path).
3. **x11aic** (x11mdl:253, Kpart==2 only) — the AIC keep/drop test (see below).
   After it, Xtdtst/Xeastr/Xuser are zeroed and the rejected groups are gone from
   the design; the winning easter window persists via `Aicind`.
4. **idotlr** (x11mdl:424-451) — automatic AO-outlier ID on the irregular
   regression. `idotlr` is ALREADY PORTED (M4 regARIMA engine); needs x11reg
   wiring: cvec(AO/LS/TC)=Critxr, lxao=T/lxls=lxtc=F (AO-only), Begxot/Endxot
   span, Cvxrdc reduction, svolit(LCLOSE) save at Kpart==3. Produces the 7 AOs.
5. **regx11 final** + rgtdhl + x11ref factor build -> b16 (Kpart==2) / c16
   (Kpart==3) (ported for the TD path; design now carries TD+Easter[15]+AOs).

## x11aic.f (604) — the AIC test, easter path
Header removes any existing td/easter/user group (dlrgef loop, :112-146). Then
per family (this spec: easter only):
- **jacobian adj** (:87-106): for mult, `jadj = sum log Xnstar(i)` over
  non-excluded rows -- added to Lnlkhd inside the AICC (N(t)* transform jacobian).
- **easter loop** (:~300-390): baseline no-easter fit (`aicnhol`), then for each
  window in Xeasvc = {1, 8, 15}: `addeas(win+Easidx, Easidx, 1)` -> regvar ->
  regx11 -> rgtdhl -> `xrlkhd(aichol, Nxcld)`; emit `aictest.xe.aicc.easterNN`.
  Track min AICC -> `aicind` (winning window) if it beats noeaster, else drop.
- **apply** (:432-451): dlrgef the test easter, re-`addeas(aicind+Easidx,...)` the
  winner, final regvar. Sets Easgrp so the group persists into Kpart==3.

## xrlkhd.f (56) — AICC (TRIVIAL, port first)
`nefobs = Nspobs - Nxcld; dnp = Ncxy - (#fixed reg); Aicc = -2*(Lnlkhd -
nefobs*dnp/(nefobs-dnp-1))` when `Var>0 & Convrg & nefobs>dnp+1`, else DNOTST.
`Lnlkhd`/`Var`/`Convrg`/`Ncxy` already produced by the ported `regx11`.

## addeas.f (45) — easter[window] regressor build
Adds one Easter contrast column for a given window width (1/8/15). Reuses the
easter builder already in the regARIMA reg path (easter{} for the model); confirm
the x11reg call maps to the same `easter`/`eastr` contrast with the window arg.

## Increments (each committable + gated)
1. **DONE (bit-exact).** xrlkhd (already ported) + addeas (already ported) +
   x11aic easter branch. Gates the 4 `aictest.xe.aicc.*` canaries + `window: 15`
   in test_x11regression_tables.py (RTOL 1e-9; measured ~1e-13). Pieces:
   `x11aic_easter` in x11reg.cpp (the window loop reusing regvar/regx11/xrlkhd);
   the `md.lnlkhd`/`md.convrg` + var-floor added to the C++ regx11 (the ENABLER —
   the -td spec never exercised var/lnlkhd, only the coefficients, so the fit's
   variance was untested until now); the aictest parse wiring in gt_x11regression
   (argidx 19 -> x11log.xeastr + the default Xeasvc {0,1,8,15}/Neasvx=4); the
   `aicc_xe` harness emit.
   **DECISIVE gotcha (editor.f:1729-1736):** the 2.5-sigma tdxtrm exclusion is
   used ONLY for a TD-only irregular regression with no easter/holiday/AO. When
   easter is present the first `IF` fails (`.not.Xeastr`), so `Otlxrg=T` and
   `Sigxrg` stays 0 -> tdxtrm is SKIPPED and extreme values are deferred to the
   automatic AO outlier ID. x11mdl_td had hardcoded `sigxrg=2.5`, which wrongly
   excluded 10 rows at the AIC test (the -td spec's bit-exactness hid it because
   -td only validates the coefficients, not the AIC-test var/nefobs/jadj). Fix:
   `sigxrg = ctx.x11log.xeastr ? 0.0 : 2.5`, gate tdxtrm on `sigxrg>0`.
2. **DONE, BIT-EXACT (~5e-15).** idotlr x11reg (Lxreg) wiring -> the 7 AOs -> xrm
   (14 cols, 156 rows), b16, c16 all gate in test_x11regression_tables.py. Pieces:
   - **Shared idotlr gains an `lxreg=false` param** (core/src/regarima/outlier.cpp).
     lxreg=true switches ONLY the OLS specifics (M4 callers pass the default, so the
     regARIMA outlier gates stay byte-identical): (a) the design copy is used
     unfiltered -- no armafl on txa OR on otlvar (idotlr.f:340-345,415-420),
     na=Nspobs; (b) robust mse from the raw residuals (medabs A(1)); (c) each
     re-estimation is `regx11` OLS instead of `rgarma`. **THE BUG that cost a
     debug round:** the otlvar armafl at the forward-scan (outlier.cpp:774) was
     UNCONDITIONAL -- for lxreg it corrupted the AO regressor, so idotlr found 0
     AOs. Gate it on `!lxreg`.
   - **regx11 exposes residuals + effective-obs** via optional out-params
     (aout/naout/nefout) so idotlr's robust-mse starting values + the re-fit `a`
     come from the OLS (x11mdl.f:416 `regx11(a)`). rgtdhl is a no-op here (Xhlnln=F,
     linear easter) so it is not ported.
   - **x11mdl_td setup** (editor.f:1734-1757): easter present -> Otlxrg=T,
     `Critxr=setcv(nobxot, Cvxalf)` with **Cvxalf default = PT5 = 0.05** (NOT 0.5 --
     PT5 is 0.05D0 in gtinpt.f; the 0.5 misread gave critxr 3.15 vs the oracle's
     3.89). AO-only, add-one (Ladd1x default = T), full model span. After idotlr,
     one regvar rebuilds the Nobspf design so the AO columns carry forecast rows;
     no re-fit (idotlr's last regx11 stands).
   - **x11ref_td extended to fold fhol** (the Easter holiday factor) into Faccal
     (x11ref.f:47-95 mult path). Needed for c16: the B-iteration `divsub(Sti,Faccal)`
     must remove TD*Easter or the C-iteration TD fit diverges. Holiday cols
     (PRGTEA/EC/LD/TH/UH) -> local Fhol -> mulref into Fcal; no-op when absent (the
     bare-TD / pritd path is unchanged). NOTE: B finds 1 AO, C finds all 7 -- that
     is the correct oracle behavior (B/C irregulars differ); b16/c16 both gate.
3. (later) aictest **td** variant + **user** variant; additive/pseudo-add;
   Cvxtyp corrected-critical-value variant; the excluded-row (nxcld>0) lxreg case.

## Reuse ledger
PORTED & reused: regvar, regx11, rgtdhl, tdxtrm, xrgtrn, loadxr, idotlr (M4),
easter contrast builder, dlrgef/addtd (verify). NEW: xrlkhd, addeas x11reg call,
x11aic orchestration (easter subset first), idotlr x11reg wiring, the udg canary
emit. Harness: emit aictest.xe.aicc.* + window to stdout for the gate.
