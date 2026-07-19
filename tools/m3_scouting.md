# M3 Scouting Report — regARIMA Estimation Engine

Scouted 2026-07-18 against `D:\code_projects\x13new\oracle\fortran` (v1.1 b61 vendored source).
Scope: ARMA filtering, likelihood, lmdif optimizer, estimate/forecast drivers.

## 1. Entry points

### (a) Estimation

```
aaamain.f:213  PROGRAM x12a          CALL x12run
x12run.f:181                         CALL x11ari
x11ari.f:133                         CALL arima(havmdl,extok,Lx11,Lseats,Lgraf)
arima.f:705    (main path)           CALL rgarma(Lestim,Mxiter,Mxnlit,Lprtit,a,na,nefobs,ltmp)
arima.f:859    (re-estimate after outlier removal)  CALL rgarma(...)
```

`rgarma` (rgarma.f:2) is **the** estimation engine — IGLS outer loop (OLS/GLS for
regression betas via `olsreg`, nonlinear ARMA via `lmdif`). Everything else that
estimates a model goes through it too: `automd.f` (11 sites), `automx.f:504,845`,
`amdid.f:354`, `amdid2.f:78`, `idotlr.f:883,1000`, `tdaic.f`, `lomaic.f`, `easaic.f`,
`usraic.f`, `trnaic.f`, `chkchi.f`, `chkmu.f`, `pass2.f`, `tstmd1.f`, `testodf.f`,
`iddiff.f`. Porting `rgarma` + its subtree unlocks all of them.

Inside `rgarma` the estimation stack is:

```
rgarma → strtvl, setmdl                 (pack estprm, starting values, difference Xy)
       → armafl                          (exact ARMA filter = likelihood evaluation)
       → olsreg → dppfa                  (GLS betas given current ARMA filter)
       → resid, yprmy                    (residuals, objfcn = a'a * exp(Lndtcv/n))
       → stpitr                          (IGLS convergence test)
       → lmdif(fcnar, ...)               (nonlinear ARMA step; fcnar → upespm → armafl)
       → fdjac2, qrfac, covar            (post-convergence ARMA covariance matrix)
```

Post-estimation reporting from `arima.f`: `prterr` (arima.f:711), `prtmdl` (:916),
`armacr` (:963, correlation matrix), `prlkhd` (:968, AIC/AICC/BIC/HQ into `lkhd.cmn`),
`armats` (t-stats, via automd.f:343 / tstmd1.f:70 / tstmd2.f:44).

### (b) Forecasts

```
arima.f:1157   CALL regvar(...)                    (rebuild Xy incl. forecast rows — M2)
arima.f:1165   CALL prtfct(...)   [Nfcst>0]
  prtfct.f:90    CALL fcstxy(fctori,Nfcst,fcst,fcstse,rgvar)   ← forecast math
arima.f:1174   CALL mkback(...)   [Nbcst>0]
  mkback.f:66    CALL fcstxy(fctori,Nbcst,Bcst,fcstse,rgvar)   ← backcasts (series reversed, mkback.f:65 revrse)
```

`fcstxy` (fcstxy.f:2) is the pure forecast kernel: filters Xy via `armafl`
(fcstxy.f:100), builds full AR/MA polynomials via `polyml`, psi-weights via `ratpos`,
recursive forecasts, SEs from psi-weights + `dppsl`/`yprmy` regression term.
`prtfct`/`mkback` are print/save wrappers (invfcn back-transform, CI, tables).
Automdl's out-of-sample forecast-error test uses `amdfct` (amdfct.f:229 rgarma,
:240 fcstxy), called from arima.f:364,733. `nofcst` (arima.f:367) just zeroes
Nfcst/Nbcst and re-calls `regvar`.

## 2. Call graph (intra-project + vendored numerics only)

Utilities already ported per `tools/ported.yaml`: `eltlen`, `setdp`, `gtarma`. `gtfcst` is gated (spec input).

| Routine | Calls |
|---|---|
| **rgarma** (rgarma.f) | strtvl, setmdl, copy, armafl, chkrt2, arflt, olsreg, resid, yprmy, maxvec, stpitr, prtitr, lmdif(fcnar), fcnar, fdjac2, upespm, qrfac, covar, savitr, dpeq, dpmpar, errhdr, abend, writln |
| **fcnar** (fcnar.f) | upespm, copy, armafl, setdp, scrmlt, getstr, prtitr, errhdr |
| **armafl** (armafl.f) | chkrts, intgpg, mltpos, ratpos, copy, uconv, euclid, xpand, setdp, exctma, xprmx, dppfa, logdet, arflt, ddot, dsolve |
| **arflt** (arflt.f) | maxlag |
| **exctma** (exctma.f) | copy, dsolve, ratneg, ratpos, scrmlt, setdp |
| **intgpg** (intgpg.f) | copy, dppfa, logdet, ratneg, ratpos, under0 |
| **setmdl** (setmdl.f) | setdp, roots, getstr, abend |
| **strtvl** (strtvl.f) | (leaf) |
| **upespm** (upespm.f) | (leaf) |
| **chkrts** (chkrts.f, LOGICAL FUNCTION) | setdp |
| **chkrt2** (chkrt2.f) | roots, setdp, getstr, writln |
| **roots** (roots.f) | revrse, rpoly |
| **olsreg** (olsreg.f) | xprmx, dppfa, copy, daxpy, errhdr, abend |
| **resid** (resid.f) | dcopy, daxpy, errhdr, abend |
| **stpitr** (stpitr.f, LOGICAL FUNCTION) | errhdr, abend |
| **lmdif** (lmdif.f) | fcn (=fcnar), fdjac2, upespm, prtitr, qrfac, lmpar, dpmpar, enorm, dpeq |
| **fdjac2** (fdjac2.f) | fcn, dpmpar, dpeq |
| **lmpar** (lmpar.f) | qrsolv, dpmpar, enorm, dpeq |
| **qrfac** (qrfac.f) | dpmpar, enorm, dpeq |
| **qrsolv** (qrsolv.f) | dpeq |
| **covar** (covar.f) | (leaf; MINPACK covar, has notset/srslen/model .prm includes only) |
| **fcstxy** (fcstxy.f) | copy, armafl, eltlen†, polyml, setdp, ratpos, resid, dppsl, yprmy |
| **prtfct** (prtfct.f) | fcstxy, regvar(M2), copy, addate, wrtdat, lgnrmc, numfmt, subset, itoc, dtoc, eltfcn, invfcn, scrmlt, opnfil, fclose, errhdr, abend |
| **mkback** (mkback.f) | fcstxy, regvar(M2), copy, revrse, lgnrmc, numfmt, wrtdat, opnfil, itoc, dtoc, fclose, invfcn, eltfcn, scrmlt, addate, errhdr, abend |
| **amdfct** (amdfct.f) | rgarma, fcstxy, regvar(M2), ssprep, rdotlr, dlrgef, restor, setdp, copy, cpyint, daxpy, eltfcn, revrse, invfcn, subset, dfdate, addate, getstr, writln, abend |
| **nofcst** (nofcst.f) | setxpt, regvar(M2) |
| **armacr** (armacr.f) | isfixd, getstr |
| **armats** (armats.f) | writln, abend |
| **dsarma** (dsarma.f) | (leaf; string builder) |
| **rvarma** (rvarma.f) | getstr, itoc |
| **prarma** (prarma.f) | (leaf; writes spec echo) |
| **prlkhd** (prlkhd.f) | dfdate, opnfil, abend, errhdr, fclose |
| **xrlkhd** (xrlkhd.f) | dpeq, strinx, abend |
| **prtitr** (prtitr.f) | yprmy, savitr |
| **savitr** (savitr.f) | opnfil, itoc, dtoc, fclose, abend |
| leaf numerics | yprmy, xprmx (call under0), maxvec, maxlag, logdet, uconv, euclid, xpand, mltpos, ratpos, ratneg, dsolve (calls daxpy), dppfa, dppsl (calls daxpy), polyml (calls insort), rpoly, dpmpar, enorm, dpeq, copy, dcopy, daxpy, ddot, scrmlt, revrse |

† eltlen already ported.

`under0` toggles floating-underflow trapping (platform shim) — port as a no-op.

## 3. COMMON block dependencies

Core state (must be live in `X13Context` before M3):

- **model.cmn** `/cmdl/` — model structure: `Arimal, Arimaf, Opr, Mdl, Nopr, Nb, Ncxy, Nestpm, Nintvl, Nextvl, Mxarlg, Mxdflg, Mxmalg, Lar, Lma, Lextar, Lextma, Regfx, Iregfx, Imdlfx, Tol, Nltol, Nltol0, Stepln, Ap1, Sp, Lcalcm, Lprier`, titles.
- **mdldat.cmn** `/cmdldt/` — data-dependent state: `Xy, B, Arimap, Armacm, Chlgpg, Chlvwp, Chlxpx, Matd, Var, Lnlkhd, Lndtcv, Nspobs, Nliter, Nfev, Nlwrk, Armaer, Convrg, Sngcol, Prbfac, Begspn, Eick`.
- **series.cmn** `/csrs/` — `Dnefob, Lrgrsd, Tsrs(PLEN)` (Tsrs = regression residuals fed to fcnar; Lrgrsd = big-residual penalty).
- **units.cmn**, **error.cmn** (`Lfatal`), **stdio.i** — I/O units + fatal flag.

Per-routine .cmn usage (from INCLUDE scan):

| Routine | .cmn blocks |
|---|---|
| rgarma | tbllog, hiddn, series, model, mdldat, units, error + local `/lgiter/` (Frstcl,Scndcl — declared inline at rgarma.f:110, shared with prtitr) |
| fcnar | series, units, model, mdldat, error |
| armafl, exctma, intgpg, strtvl, upespm, chkrts, prarma | model, mdldat |
| setmdl | model, mdldat, units, error |
| chkrt2 | model, mdldat, units, error |
| olsreg, resid | units |
| stpitr | units (+ SAVE'd `oldobj` local state) |
| lmdif | error only (Lfatal check after prtitr) |
| fdjac2, lmpar, qrfac, qrsolv, covar | none |
| fcstxy | model, mdldat, error, units |
| prtfct | model, mdldat, tbllog, units, rev, adj, revsrs, hiddn, error, x11log, x11fac, prior, priusr, savcmn, seatad |
| mkback | model, mdldat, arima, x11log, extend, hiddn, adj, x11adj, x11fac, priusr, error, x11opt, savcmn, tbllog, units |
| amdfct | model, mdldat, arima, prior, error, usrreg, units, fxreg |
| nofcst | arima, prior, extend |
| armacr | model, mdldat, units, error |
| armats | model, mdldat, units |
| dsarma | model, units |
| rvarma | model, mdldat, error (+rev.prm) |
| prlkhd | model, mdldat, **lkhd**, units, hiddn, lzero, extend, x11adj, x11fac, x11log, x11opt, svllog |
| xrlkhd | model, mdldat |
| prtitr | series, units, tbllog, model, error |
| savitr | model, mdldat, savcmn |

**lkhd.cmn** `/lkhd/` = `Aic, Aicc, Bic, Bic2, Hnquin, Olkhd, Eic` — pure output of
prlkhd/xrlkhd, consumed by AIC-test drivers. Also note **arima.cmn** carries the
estimation options `Mxiter, Mxnlit, Lestim` read by the callers of rgarma.

Kernel takeaway: the estimation kernel proper (rgarma→lmdif→fcnar→armafl) only
needs `model.cmn`, `mdldat.cmn`, `series.cmn`, `units/error`, `hiddn` (Issap/Irev
for `gudrun`), and `tbllog` (print/save flags). The heavyweight prtfct/mkback/prlkhd
table wrappers drag in the X-11 output commons and can be deferred to the end of M3.

## 4. The lmdif interface

**lmdif.f is NOT verbatim MINPACK** — Census modified it (header still says MINPACK
March 1980). Diffs from stock `lmdif`:

1. Signature (lmdif.f:188): `lmdif(fcn, M, N, X, Fvec, Lauto, Gudrun, Ftol, Xtol,
   Gtol, Mxiter, Epsfcn, Diag, Mode, Factor, Nprint, Info, Nliter, Nfev, Fjac,
   Ldfjac, Ipvt, Qtf, Wa1, Wa2, Wa3, Wa4)` — `MAXFEV` replaced by `Mxiter`
   (ARMA-iteration cap), plus pass-through flags `Lauto`/`Gudrun` and **cumulative**
   counters `Nliter`, `Nfev` (not zeroed on entry; `oldfev=Nfev` at lmdif.f:217,
   `begitr=Nliter` at :247 — IGLS calls accumulate across rounds).
2. `maxfev = max(Mxiter,200)*(N+1)` internally (lmdif.f:227); `Info=5` on
   `Nliter>=Mxiter` **or** `Nfev-oldfev>=maxfev` (lmdif.f:461-463).
3. Callback signature (lmdif.f:239,378): `fcn(M, N, X, Fvec, Lauto, Gudrun, Iflag,
   Lckinv)` — the stock `fcn(m,n,x,fvec,iflag)` plus three flags. `Lckinv=.false.`
   for the initial evaluation and inside fdjac2; `.true.` for trial steps in the
   inner loop (lmdif.f:378) — that's where invertibility is enforced.
4. `CALL upespm(X)` after every fdjac2 (lmdif.f:265) and after every **rejected**
   step (lmdif.f:444) — re-syncs `Arimap` in mdldat.cmn to X, because fcnar writes
   trial parameters into the shared model state via upespm.
5. `prtitr` iteration printing (lmdif.f:272,498), `dpeq` used for all `==0` tests,
   `error.cmn` Lfatal early-return (lmdif.f:273).

**Parameter mapping**: `setmdl` (called from rgarma.f:158) packs the free (non-fixed)
AR/MA coefficients from `Arimap`/`Arimaf` into `estprm(1..Nestpm)`; `upespm`
(fcnar.f:80) scatters `estprm` back into `Arimap` slots. Differencing is folded into
Xy by setmdl, so lmdif only sees ARMA coefficients.

**Residual vector** (`Fvec`, length `M = Na = Nefobs + Mxmalg` when AR or MA present,
rgarma.f:217): fcnar copies `Tsrs` (regression-adjusted series, y − Xb, set at
rgarma.f:372 via `resid`) into A, runs `armafl` (exact ARMA filter: difference →
conditional AR filter → exact MA filter via G'G cholesky → w_p correction), then
scales by `exp(Lndtcv/(2*Dnefob))` (fcnar.f:141-142) — so `‖Fvec‖² = a'a·|Σ|^(1/n)`
= the deviance/concentrated-likelihood objective. On filter failure (non-invertible
roots etc.), fcnar sets every residual to `Lrgrsd` (fcnar.f:136; initialized at
rgarma.f:143-345 to max|a|·|Σ|^(1/2n)) — a penalty wall that keeps lmdif inside the
invertibility region.

The true MINPACK-verbatim files are `lmpar.f`, `qrfac.f`, `qrsolv.f`, `enorm.f`,
`dpmpar.f`, `covar.f` (only cosmetic edits: IMPLICIT NONE, `dpeq` instead of `==`).
`fdjac2.f` has the extra Lauto/Gudrun/Lckinv pass-through args but is otherwise stock.
Port lmdif/fdjac2 **as found in this repo**, not from upstream MINPACK.

## 5. Ordered port list (leaf-first)

Each tier depends only on earlier tiers (+ already-ported eltlen/setdp/gtarma and M2's
regvar/getstr/writln/opnfil/table machinery).

**Tier 0 — scalar/vector leaves** (trivial, unit-test vs oracle values):
`dpmpar`, `dpeq`, `copy`, `dcopy`, `daxpy`, `ddot`, `scrmlt`, `revrse`, `maxvec`,
`maxlag`, `insort`, `under0` (no-op), `enorm`

**Tier 1 — linear algebra / polynomial leaves**:
`yprmy`, `xprmx`, `logdet`, `dppfa`, `dppsl`, `dsolve`, `uconv`, `euclid`, `xpand`,
`mltpos`, `ratpos`, `ratneg`, `polyml`, `arflt`, `rpoly` (Jenkins–Traub, 322 lines)

**Tier 2 — model-state helpers**:
`roots`, `chkrts`, `strtvl`, `upespm`, `setmdl`, `olsreg`, `resid`, `chkrt2`, `stpitr`

**Tier 3 — the filter** (first big parity target):
`intgpg`, `exctma`, `armafl` — parity-test: fixed Arimap + Xy → residual vector +
`Lndtcv` at rtol 1e-12 (pure algebra, should match tightly)

**Tier 4 — optimizer**:
`qrfac`, `qrsolv`, `lmpar`, `covar`, `fdjac2`, `fcnar`, `prtitr`, `savitr`, `lmdif`

**Tier 5 — estimation driver**:
`rgarma` — parity: coefficient estimates, `Var`, `Lnlkhd`, `Nliter`, `Nfev`, `Convrg`

**Tier 6 — likelihood stats + reports**:
`xrlkhd`, `prlkhd`, `armats`, `armacr`, `dsarma`, `prarma`, `rvarma`

**Tier 7 — forecasting**:
`fcstxy`, then `nofcst`, `prtfct`, `mkback`, `amdfct` (these need M2 `regvar` +
table machinery; `lgnrmc/invfcn/eltfcn/numfmt/subset` come from M2 transform cluster)

## 6. Parity risks

1. **Penalty-wall discontinuity** (fcnar.f:136): one bit of FP difference near the
   invertibility boundary flips a trial step between "real residuals" and "all =
   Lrgrsd", changing the whole optimizer trajectory. Models with near-unit MA roots
   will match to 1e-8 only if Tiers 0–3 are bit-faithful. Test rgarma parity on
   coefficient values, not iteration traces, for such models — but do compare
   `Nliter`/`Nfev` on benign models as a canary.
2. **enorm.f accumulation**: MINPACK enorm uses the 3-bin (small/mid/large) summation,
   not `sqrt(sum(x²))`. Must be ported verbatim or step norms (hence step acceptance)
   drift.
3. **dpeq-based exact comparisons** throughout lmdif/qrfac/lmpar (e.g. lmdif.f:287,
   299) — keep exact `==`-style semantics (dpeq is a tolerance/equality helper; port
   its exact definition first and reuse it).
4. **Cumulative counters**: `Nliter`/`Nfev` accumulate across IGLS rounds and across
   *repeated rgarma calls* in AIC-testing/automdl paths (rgarma passes
   `Nliter+tnlitr` as Mxiter, rgarma.f:374). Getting the cumulative bookkeeping wrong
   changes termination (`Info=5`) on hard models.
5. **Hidden state / SAVE variables**: `stpitr` SAVEs `oldobj` between calls
   (stpitr.f:115); `armafl` SAVEs `nextma` (armafl.f:33) between the init call and
   later non-init calls (fcstxy calls armafl with `Linit=F` relying on state left by
   estimation); `/lgiter/ Frstcl,Scndcl` shared between rgarma and prtitr. These must
   become explicit ctx members, keyed to the same lifetimes.
6. **Tolerance scaling** (rgarma.f:214-234): `devtol = 2/n·Tol`,
   `tnltol = 2/n·Nltol0` switching to `2/n·Nltol` after iter 2 (rgarma.f:370).
   Reproduce exactly, including the `Nb>0` vs `Nb=0` branch (no regression ⇒ single
   lmdif pass with overall Tol/Mxiter).
7. **Order-sensitive reductions**: `Lndtcv` is accumulated as a sum of log-dets
   (intgpg + armafl:186); `ddot` with stride in the D'a correction (armafl.f:240);
   Cholesky `dppfa` inner loops; `objfcn = a'a·exp(Lndtcv/n)` (rgarma.f:338). Keep
   loop order identical; do not "improve" to fused/BLAS forms.
8. **rpoly (roots.f → rpoly.f)**: Jenkins–Traub is iteration-count-sensitive; used in
   setmdl (start-value root check) and chkrt2/prtrts (root reporting, error paths).
   Small root differences can flip the `chkrts`/`Armaer` error classification.
   Port verbatim; parity-test on polynomials with root moduli near 1.
9. **fdjac2 step size**: `h = sqrt(max(Epsfcn,epsmch))·|x|` with `h=eps` when x=0 —
   Epsfcn comes in as `Stepln` (model.cmn, user-settable). Forward differences mean
   the Jacobian inherits any residual-vector divergence amplified by 1/h ≈ 1e8 —
   the main reason Tier 3 needs the tightest parity budget.
10. **Var flush** (rgarma.f:424): `Var < 2·dpmpar(1) ⇒ Var=0` then `Lnlkhd=0` —
    exact-zero semantics must be preserved; `dpmpar(1)` must be IEEE-double
    2.220446049250313e-16.
11. **EQUIVALENCE workspace overlay** (rgarma.f:100-104): diag/qtf/wa1..wa4/tmpa all
    alias `txy`. Safe to replace with separate arrays in C++ (no live overlap), but
    verify `tmpa` (post-convergence fcnar call, rgarma.f:438) doesn't rely on wa4
    contents — it starts at `txy(5·PARIMA+PA+1)`, disjoint. Document, don't replicate.
12. **prtfct/mkback rounding**: output formatting (`numfmt`, Kdec) rounds for tables —
    compare saved-table numbers at print precision, raw fcstxy vectors at 1e-8.

## 7. Spec→estimate integration gap (post-rgarma roadmap)

`rgarma` + its numeric stack (Tiers 0–6) are ported and oracle-verified via
hand-set common state (`ref_rgarma*.f` / test_numeric). To drive rgarma from a
**real corpus spec**, the missing tissue between `run_m2` (driver/run_pre_model.cpp)
and a `rgarma` call is small and now mapped. `run_m2` already parses the spec,
builds the ARMA operators (getmdl → insopr/mkoprt/maxlag sets Mdl/Opr/Arimal/
Arimaf/Oprfac/Nopr + Mxarlg/Mxdflg/Mxmalg), and builds `[X:y]` into
`ctx.mdldat.xy` with `Ncxy` (regvar). What rgarma additionally needs:

- **Estimate-spec defaults** (gtinpt.f:271–279) — **DONE** (gtinpt.cpp init):
  `Mxiter=1500`, `Mxnlit=40`, `Stepln=0`, `Tol=DFTOL(1e-5)`, `Nltol0=100·DFTOL`,
  `Nltol=DFTOL`, `Lextar=T`, `Lextma=T`, `Lestim=T`.
- **Model-finalize block** (gtinpt.f:220) — **DONE** (`mdlfin`, gtinpt.cpp; test
  `mdlfin:`): `Lar=Lextar∧Mxarlg>0`, `Lma=Lextma∧Mxmalg>0`; `Lextar`:
  `Nintvl=Mxdflg`,`Nextvl=Mxarlg+Mxmalg`; else `Nintvl=Mxdflg+Mxarlg`,
  `Nextvl=Lextma?Mxmalg:0`. Called at end of gtinpt.
- **`estimate{}` reader** (gtestm.f) — **DONE** (`gt_estimate`, readers_spec.cpp;
  test `estimate{}:` drives it through `parse_spec`). Applies maxiter/maxnliter/
  tol/nltol/parms/exact/step + the tol-reconciliation tail; output/AIC/model-file
  args (print/save/savelog/file/fix/k/removeconstant/outofsample) are token-
  consumed with state application deferred to their milestones.
- **`Nb`** (regression β count): confirm provenance — gtinpt init sets
  `Nb=Ncoltl`, regvar sets `Ncxy`. Verify `Nb`/`Ncxy` post-regvar equal what
  rgarma's olsreg-vs-yprmy branch + `resid` column span expect.
- **Already handled inside rgarma:** `Tsrs` (written by resid from Xy), `Dnefob`,
  `Lndtcv`, `Nefobs=Nspobs-Nintvl`. **Default-OK from zero-init:** `Issap/Irev`
  (→ gudrun=T), `Lprier/Lprtit/Lhiddn` (deferred prints).

**Remaining integration step (the M3 headline):** an end-to-end test — `run_m2`
on an airline spec (e.g. `02-airline-log-td-easter.spc`, or a self-contained
inline-data spec) → `rgarma` → compare the estimated ARMA coefficients / `Var` /
`Lnlkhd` against the oracle `.udg` (`arima.ar`/`arima.ma`/`likelihood.*`) via the
x13compare harness. Open questions to resolve there: (a) is `Tsrs` seeded from
the transformed series before rgarma, or does run_m2 need to copy `trnsrs`→`Tsrs`?
(b) does Xy need differencing applied first, or does armafl's internal filtering
suffice given `Nintvl`? (c) `Nb`/`Ncxy` consistency post-regvar. That is the
first **real-data** end-to-end estimation parity.
