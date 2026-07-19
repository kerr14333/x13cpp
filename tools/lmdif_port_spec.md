# PORT SPEC — lmdif (Census-modified MINPACK LM core)

Source: `oracle/fortran/lmdif.f` (530 lines, subroutine at :188).
Target: `core/src/numeric/minpack.{hpp,cpp}`, beside qrfac/qrsolv/lmpar/fdjac2.
Audited 2026-07-19. All line numbers below are `lmdif.f` unless noted.

---

## 0. Resolved judgment calls (summary)

1. **upespm → optional `sync` callback.** lmdif's two `CALL upespm(X)` sites
   (:265, :444) have **zero effect on lmdif's own numerics** — every `fcn`
   (fcnar) call re-syncs model state itself as its first action (fcnar.f:80
   `CALL upespm(Estprm)`), so any value lmdif computes is independent of the
   syncs. They exist to establish the exit invariant *"Arimap == returned X on
   lmdif exit"*, which rgarma-side consumers rely on (fcnar.f:19-21: "ARflt
   filters ... using parameter estimates saved during the last fcnar call";
   rgarma reads Arimap via arflt/chkrt2/covar after lmdif without necessarily
   calling fcnar first). Verdict: **semantically required for the module
   boundary, numerically a no-op inside lmdif** → thread as an optional
   `std::function<void(const double*)>` defaulting to empty; empty ⇒ skip
   (exactly what the oracle ref driver's no-op stub does). rgarma's port passes
   `[&ctx](const double* x){ upespm(ctx, x); }`.
2. **prtitr → optional callback returning `bool` (Lfatal), plus keep `nprint`.**
   rgarma.f:145-149 sets `nprint = Lprtit ? 1 : 0` — printing IS a reachable
   production path (user's `print` spec), but the .out engine is deferred.
   prtitr is write-only w.r.t. lmdif numerics (prtitr.f touches no argument of
   lmdif; its only feedback is `Lfatal`, set only inside savitr on save-table
   I/O failure). Skipping it **cannot change numerics**; it can only change the
   Lfatal early-return path (:273), which never fires absent I/O failure.
   Thread an optional callback so the signature won't churn when the print
   milestone lands; empty callback ⇒ behave as `nprint<=0`.
3. **Nliter/Nfev are cumulative in/out `int&`.** NOT zeroed on entry
   (`oldfev=Nfev` :217, `begitr=Nliter` :247). Contract documented in §2.
4. **Control flow: two nested `while(true)` + one C++ `goto termination`.**
   `GO TO 20` is reached from 6 places across two loop nests and two guard
   `IF`s — a real `goto` is the only unambiguous mapping (flag chains are the
   bug farm here). `GO TO 10` collapses to `break` out of the inner loop
   (lmpar precedent). Full skeleton in §3. The Lfatal path (:273) is a bare
   `return`, NOT `goto termination` — it skips the `info=iflag` fixup and the
   final prtitr.

---

## 1. Recommended minpack.hpp additions

```cpp
// Model-state sync hook: lmdif calls it with the CURRENT accepted parameter
// vector x after every Jacobian (fdjac2 leaves the model synced to a perturbed
// x, lmdif.f:265) and after every rejected trial step (lmdif.f:444), so that
// on exit the model state matches the returned x. It never feeds back into
// lmdif's numerics (the objective re-syncs on every call); empty = skip.
// rgarma passes [&ctx](const double* x){ upespm(ctx, x); }.
using MinpackSync = std::function<void(const double* x)>;

// Iteration-print hook standing in for prtitr.f (lmdif.f:272,498). Args mirror
// prtitr(A,Na,Parms,Nparms,Itrlbl,Iter,Nfev). Return true iff a fatal error
// occurred (Lfatal), which makes lmdif return immediately (lmdif.f:273).
// Empty = no printing (the Nprint<=0 behavior); print engine deferred.
using MinpackPrtitr = std::function<bool(const double* fvec, int m,
                                         const double* x, int n,
                                         const char* itrlbl, int nliter,
                                         int nfev)>;

// lmdif.f: Census-modified MINPACK Levenberg-Marquardt (forward-difference
// Jacobian). Minimizes ||fcn(x)||^2. NOT stock MINPACK: MAXFEV is derived
// internally from mxiter, fcn carries lauto/gudrun/lckinv, nliter/nfev are
// CUMULATIVE in/out counters, and dpeq() replaces every ==0 test.
void lmdif(const MinpackFcn& fcn,  // objective (fcnar in production)
           int m,                  // residual count (>=n); aliased through fcn like fdjac2
           int n,                  // number of parameters
           double* x,              // in: start, out: final estimate (n)
           double* fvec,           // out: residuals at final x (m)
           bool lauto,             // pass-through run-mode flag for fcn
           bool gudrun,            // pass-through run-mode flag for fcn
           double ftol,            // relative sum-of-squares tolerance (info=1)
           double xtol,            // relative step tolerance (info=2)
           double gtol,            // gradient-cosine tolerance (info=4)
           int mxiter,             // ARMA-iteration cap: info=5 when nliter>=mxiter (0 = no cap); also maxfev=max(mxiter,200)*(n+1)
           double epsfcn,          // forward-difference relative step (fdjac2)
           double* diag,           // scale factors (n); in/out for mode!=2 (set from column norms), in for mode==2 (must be >0)
           int mode,               // 1 = auto-scale, 2 = user diag
           double factor,          // initial step-bound factor (rgarma: 100.0)
           int nprint,             // >0 enables the prtitr hook (rgarma: Lprtit?1:0)
           int& info,              // out: termination code 0..8, or negative iflag on user abort
           int& nliter,            // in/out CUMULATIVE successful-iteration counter (not zeroed)
           int& nfev,              // in/out CUMULATIVE fcn-evaluation counter (not zeroed)
           double* fjac,           // out m-by-n (column-major, leading dim ldfjac): final QR of the Jacobian (R in upper triangle, rdiag on diagonal)
           int ldfjac,             // leading dimension of fjac (>= m)
           int* ipvt,              // out: qrfac pivot permutation (n)
           double* qtf,            // out: first n components of Q'*fvec (n)
           double* wa1, double* wa2, double* wa3,  // work (n each)
           double* wa4,            // work (m)
           const MinpackSync& sync = {},      // upespm hook (see above)
           const MinpackPrtitr& prtitr = {}); // prtitr hook (see above)
```

Notes:
- Parameter order 2..27 mirrors the Fortran argument list at :188-191 exactly;
  the two hooks are appended with defaults so the ref-driver/test call sites
  stay short.
- `m` by value with a local `int mm = m;` used for every post-fcn loop bound
  and `enorm(mm,...)` call — the fdjac2-port aliasing precedent (minpack.cpp
  fdjac2 comment). fcnar never actually modifies Na, but preserve the plumbing.
- lmdif's internal `iflag` is a local; it is not surfaced (rgarma never reads
  it — it reads info).

## 2. Cumulative-counter contract (judgment call 3)

- `nfev` (:217 `oldfev=Nfev`): NOT zeroed. Incremented +1 at :241 (initial
  eval), +n at :259 (each Jacobian), +1 at :379 (each trial step). The
  budget test :463 is **per-call**: `nfev - oldfev >= maxfev` where
  `maxfev = max(mxiter,200)*(n+1)` (:227).
- `nliter` (:247 `begitr=Nliter`): NOT zeroed; ++ only on accepted steps
  (:438). The cap test :461 is **absolute/cumulative**: `mxiter>0 &&
  nliter>=mxiter` — which is exactly why rgarma passes `Nliter+tnlitr` as
  Mxiter (rgarma.f:374). Do NOT "fix" this asymmetry (maxfev per-call,
  mxiter cumulative).
- `begitr` also gates the first-iteration scaling/delta init (:283, :373) and
  both prtitr guards (:271, :497). All `nliter == begitr` / `> begitr`
  comparisons are against the value captured at entry.
- C++: both `int&`. Caller (rgarma, tests) initializes to 0 once and lets
  successive lmdif calls accumulate.

## 3. Control-flow skeleton (judgment call 4) — NORMATIVE

Fortran structure: outer `DO WHILE(T)` :249, inner `DO WHILE(T)` :352,
`GO TO 10` :479 → label 10 :484 (= exit inner loop, continue outer),
`GO TO 20` (:231, :267, :343, :380, :457, :475) → label 20 :491 (termination),
plus two guard IFs (:221 validity, :242 iflag>=0) that fall through to 20.

```cpp
void lmdif(/* §1 */) {
    constexpr double ONE = 1.0, P1 = 0.1, P5 = 0.5, P25 = 0.25, P75 = 0.75,
                     P0001 = 1.0e-4, MONE = -1.0, ZERO = 0.0;   // :207-208
    double epsmch = dpmpar(1);                                   // :212
    auto FJ = [&](int i, int j) -> double& { return fjac[(j-1)*ldfjac + (i-1)]; };

    info = 0;                                                    // :214
    int iflag = 0;                                               // :215
    int oldfev = nfev;                                           // :217
    int mm = m;                       // Fortran M aliased through fcn
    int begitr = nliter;              // safe pre-init; re-set at :247 site
    double ratio = ZERO;              // read at :497 guard; only meaningful
                                      // when nliter>begitr (then always set)
    double actred, delta = ZERO, dirder, fnorm, fnorm1, gnorm = ZERO,
           par, pnorm, prered, xnorm = ZERO;

    // :221-223 input validation — on failure fall through with info=0
    if (n > 0 && m >= n && ldfjac >= m && ftol >= ZERO && xtol >= ZERO &&
        gtol >= ZERO && mxiter >= 0 && factor > ZERO) {
        int maxfev = std::max(mxiter, 200) * (n + 1);            // :227
        if (mode == 2) {                                         // :229-233
            for (int j = 1; j <= n; ++j)
                if (diag[j-1] <= ZERO) goto termination;         // :231 GO TO 20
        }
        iflag = 1;                                               // :238
        fcn(mm, n, x, fvec, lauto, gudrun, iflag, false);        // :239 lckinv=F
        nfev = nfev + 1;                                         // :241
        if (iflag >= 0) {                                        // :242
            fnorm = enorm(mm, fvec);                             // :243
            begitr = nliter;                                     // :247
            par = ZERO;                                          // :248
            while (true) {                    // :249 OUTER loop
                iflag = 2;                                       // :256
                fdjac2(fcn, mm, n, x, fvec, fjac, ldfjac, iflag,
                       epsfcn, wa4, lauto, gudrun, false);       // :257-258 lckinv=F
                nfev = nfev + n;                                 // :259
                if (sync) sync(x);                               // :265 upespm(X)
                if (iflag < 0) goto termination;                 // :267
                if (nprint > 0 && nliter > begitr) {             // :271
                    if (prtitr &&
                        prtitr(fvec, mm, x, n, "ARMA      ", nliter, nfev))
                        return;      // :272-273 Lfatal => bare RETURN,
                }                    // NOT goto termination
                qrfac(mm, n, fjac, ldfjac, true, ipvt, n, wa1, wa2, wa3); // :278
                if (nliter == begitr) {                          // :283
                    if (mode != 2) {                             // :284-289
                        for (int j = 1; j <= n; ++j) {
                            diag[j-1] = wa2[j-1];
                            if (dpeq(wa2[j-1], ZERO)) diag[j-1] = ONE;  // :287
                        }
                    }
                    for (int j = 1; j <= n; ++j)                 // :294-296
                        wa3[j-1] = diag[j-1] * x[j-1];
                    xnorm = enorm(n, wa3);                       // :297
                    delta = factor * xnorm;                      // :298
                    if (dpeq(delta, ZERO)) delta = factor;       // :299
                }
                for (int i = 1; i <= mm; ++i) wa4[i-1] = fvec[i-1];  // :305-307
                for (int j = 1; j <= n; ++j) {                   // :308-321 form qtf
                    if (!dpeq(FJ(j,j), ZERO)) {                  // :309
                        double sum = ZERO;
                        for (int i = j; i <= mm; ++i) sum = sum + FJ(i,j)*wa4[i-1];
                        double temp = -sum / FJ(j,j);            // :314
                        for (int i = j; i <= mm; ++i) wa4[i-1] = wa4[i-1] + FJ(i,j)*temp;
                    }
                    FJ(j,j) = wa1[j-1];                          // :319 restore rdiag
                    qtf[j-1] = wa4[j-1];                         // :320
                }
                gnorm = ZERO;                                    // :325
                if (!dpeq(fnorm, ZERO)) {                        // :326
                    for (int j = 1; j <= n; ++j) {
                        int l = ipvt[j-1];
                        if (!dpeq(wa2[l-1], ZERO)) {             // :329
                            double sum = ZERO;
                            for (int i = 1; i <= j; ++i)
                                sum = sum + FJ(i,j) * (qtf[i-1] / fnorm); // :332 divide INSIDE the sum
                            gnorm = std::max(gnorm, std::fabs(sum / wa2[l-1])); // :334
                        }
                    }
                }
                if (gnorm <= gtol) info = 4;                     // :341
                if (info != 0) goto termination;                 // :343
                if (mode != 2)                                   // :347-351
                    for (int j = 1; j <= n; ++j)
                        diag[j-1] = std::max(diag[j-1], wa2[j-1]);
                while (true) {                    // :352 INNER loop
                    lmpar(n, fjac, ldfjac, ipvt, diag, qtf, delta, par,
                          wa1, wa2, wa3, wa4);                   // :359-360
                    for (int j = 1; j <= n; ++j) {               // :364-368
                        wa1[j-1] = -wa1[j-1];
                        wa2[j-1] = x[j-1] + wa1[j-1];
                        wa3[j-1] = diag[j-1] * wa1[j-1];
                    }
                    pnorm = enorm(n, wa3);                       // :369
                    if (nliter == begitr) delta = std::min(delta, pnorm); // :373
                    iflag = 1;                                   // :377
                    fcn(mm, n, wa2, wa4, lauto, gudrun, iflag, true); // :378 lckinv=T
                    nfev = nfev + 1;                             // :379
                    if (iflag < 0) goto termination;             // :380
                    fnorm1 = enorm(mm, wa4);                     // :381
                    actred = MONE;                               // :385
                    if (P1*fnorm1 < fnorm)                       // :386 (fnorm1 NOT squared in the guard)
                        actred = ONE - (fnorm1/fnorm)*(fnorm1/fnorm);
                    for (int j = 1; j <= n; ++j) {               // :391-398
                        wa3[j-1] = ZERO;
                        int l = ipvt[j-1];
                        double temp = wa1[l-1];
                        for (int i = 1; i <= j; ++i) wa3[i-1] = wa3[i-1] + FJ(i,j)*temp;
                    }
                    double temp1 = enorm(n, wa3) / fnorm;        // :399
                    double temp2 = (std::sqrt(par) * pnorm) / fnorm; // :400
                    prered = temp1*temp1 + temp2*temp2/P5;       // :401 — /P5 binds temp2^2 ONLY (= t1^2 + 2*t2^2)
                    dirder = -(temp1*temp1 + temp2*temp2);       // :402
                    ratio = ZERO;                                // :407
                    if (!dpeq(prered, ZERO)) ratio = actred/prered; // :408
                    if (ratio <= P25) {                          // :412
                        double temp;
                        if (actred >= ZERO) temp = P5;           // :413
                        if (actred < ZERO)                       // :414
                            temp = P5*dirder / (dirder + P5*actred);
                        if (P1*fnorm1 >= fnorm || temp < P1) temp = P1; // :415
                        delta = temp * std::min(delta, pnorm/P1);       // :416
                        par = par / temp;                        // :417
                    } else if (dpeq(par, ZERO) || ratio >= P75) { // :418
                        delta = pnorm / P5;                      // :419
                        par = P5 * par;                          // :420
                    }
                    if (ratio >= P0001) {                        // :425 ACCEPT
                        for (int j = 1; j <= n; ++j) {           // :429-432
                            x[j-1] = wa2[j-1];
                            wa2[j-1] = diag[j-1] * x[j-1];
                        }
                        for (int i = 1; i <= mm; ++i) fvec[i-1] = wa4[i-1]; // :433-435
                        xnorm = enorm(n, wa2);                   // :436
                        fnorm = fnorm1;                          // :437
                        nliter = nliter + 1;                     // :438
                    } else {                                     // :443 REJECT
                        if (sync) sync(x);                       // :444 upespm(X)
                    }
                    // ---- convergence cascade, ORDER NORMATIVE ----
                    if (std::fabs(actred) <= ftol && prered <= ftol &&
                        P5*ratio <= ONE) info = 1;               // :449-450
                    if (delta <= xtol*xnorm) info = 2;           // :453 (overwrites 1)
                    if (std::fabs(actred) <= ftol && prered <= ftol &&
                        P5*ratio <= ONE && info == 2) info = 3;  // :455-456
                    if (info != 0) goto termination;             // :457
                    // ---- termination/stringent cascade, ORDER NORMATIVE ----
                    if (mxiter > 0 && nliter >= mxiter) info = 5; // :461 cumulative nliter
                    if (nfev - oldfev >= maxfev) info = 5;        // :463 per-call budget
                    if (std::fabs(actred) <= epsmch && prered <= epsmch &&
                        P5*ratio <= ONE) info = 6;               // :465-466
                    if (delta <= epsmch*xnorm) info = 7;         // :469
                    if (gnorm <= epsmch) info = 8;               // :471
                    if (info != 0) goto termination;             // :475
                    if (ratio >= P0001) break;   // :479 GO TO 10: successful
                    // step already handled above -> new Jacobian (outer loop).
                    // ratio < P0001 falls through: repeat INNER loop with the
                    // shrunk delta / boosted par, NO new Jacobian.
                }
                // label 10 (:484): end of outer-loop body, loop again
            }
        }
    }
termination:                                                     // label 20, :491
    if (iflag < 0) info = iflag;                                 // :491
    iflag = 0;                                                   // :492
    if (nprint > 0 && nliter > begitr && ratio >= P0001) {       // :497
        if (prtitr) prtitr(fvec, mm, x, n, "ARMA", nliter, nfev); // :498
    }                                                            // :500 RETURN
}
```

Mapping table:

| Fortran | C++ |
|---|---|
| `DO WHILE(T)` :249 / END DO :485 | outer `while (true)` |
| `DO WHILE(T)` :352 / END DO :480 | inner `while (true)` |
| `GO TO 10` :479 + `10 CONTINUE` :484 | `break` out of inner loop (outer continues) |
| `GO TO 20` :231,:267,:343,:380,:457,:475 | `goto termination;` |
| validity IF :221 / iflag IF :242 fall-through | structured `if` blocks falling to the label |
| `RETURN` :273 (Lfatal) | bare `return;` (skips :491-498 entirely) |

C++ `goto` legality: every `goto termination` only jumps OUT of scopes; all
function-scope doubles are declared (and the ones read at the label — `ratio`,
`begitr`, `mm` — initialized) before the first `goto`. Keep it that way.
Init `begitr = nliter` and `ratio = ZERO` up front: on early exits (:231,
invalid input) Fortran leaves them undefined but the :497 guard then evaluates
`nliter > begitr` = false / short-circuits, so these inits are behavior-neutral
and merely make the C++ well-defined.

## 4. dpeq / enorm / dpmpar sites (parity checklist)

`dpeq(a,b)` = Census tolerance equality from `numeric/numeric.hpp` — every
site below MUST use dpeq, never `==`:

| Line | Site |
|---|---|
| :287 | `dpeq(Wa2(j), ZERO)` → `Diag(j)=ONE` (auto-scale zero-column guard) |
| :299 | `dpeq(delta, ZERO)` → `delta=Factor` |
| :309 | `.not.dpeq(Fjac(j,j), ZERO)` (qtf Householder application guard) |
| :326 | `.not.dpeq(fnorm, ZERO)` (gnorm outer guard) |
| :329 | `.not.dpeq(Wa2(l), ZERO)` (gnorm per-column guard) |
| :408 | `.not.dpeq(prered, ZERO)` (ratio guard) |
| :418 | `dpeq(par, ZERO) .or. ratio>=P75` (step-bound grow branch) |

Plain (non-dpeq) comparisons that must stay exact as written: `Diag(j)<=ZERO`
:231, `iflag>=0` :242 / `<0` :267,:380, `gnorm<=Gtol` :341, `P1*fnorm1<fnorm`
:386 and `>=` :415, `actred>=/<ZERO` :413-414, `temp<P1` :415, `ratio<=P25`
:412 / `>=P75` :418 / `>=P0001` :425,:479,:497, all six cascade tests
:449-471, `Nliter.eq.begitr` :283,:373.

`enorm` (verbatim 3-bin port in numeric.hpp):

| Line | Call |
|---|---|
| :243 | `fnorm = enorm(M, Fvec)` |
| :297 | `xnorm = enorm(N, Wa3)` (first-iteration scaled x) |
| :369 | `pnorm = enorm(N, Wa3)` (scaled step) |
| :381 | `fnorm1 = enorm(M, Wa4)` (trial residual norm) |
| :399 | `temp1 = enorm(N, Wa3)/fnorm` (prered numerator) |
| :436 | `xnorm = enorm(N, Wa2)` (accepted scaled x) |

`dpmpar`: single call `epsmch = dpmpar(1)` :212 (truncated-literal oracle
value 2.220446e-16 semantics already handled in numeric.hpp — used raw in the
info=6/7/8 tests).

Indirect: fdjac2/qrfac/lmpar bring their own dpeq/enorm/dpmpar sites (already
ported and oracle-verified — call the ported functions, do not inline).

## 5. Oracle driver `tools/ref_lmdif.f`

Self-contained: no model commons, no regarima stack. The driver file itself
supplies **no-op stubs** for `upespm` and `prtitr` (see §7) — legitimate
because (a) with Nprint=0 prtitr is never invoked, and (b) upespm is a pure
model-state write that lmdif never reads back (§0.1), so a no-op stub leaves
the oracle numerics identical to the C++ port with empty hooks.

Three cases, run sequentially in one program. Objectives are analytic 8-arg
fcns matching `fcn(M,N,X,Fvec,Lauto,Gudrun,Iflag,Lckinv)` (tstfcn precedent in
`tools/ref_fdjac2.f:27-36`; Lauto/Gudrun/Lckinv declared LOGICAL and ignored;
Iflag never modified).

**Case A — Rosenbrock, full trajectory to convergence.**
`SUBROUTINE rosen`: `Fvec(1)=10*(X(2)-X(1)**2)`, `Fvec(2)=1-X(1)`.
Inputs: M=2, N=2, X0=(-1.2, 1.0), Ftol=1e-10, Xtol=1e-10, Gtol=0,
Mxiter=100, Epsfcn=0, Mode=1, Diag untouched, Factor=100, Nprint=0,
Nliter=0, Nfev=0, Ldfjac=2. Why: the first Gauss-Newton step from
(-1.2,1) overshoots the curved valley (trial fnorm1 ≈ 48 vs fnorm ≈ 4.9,
actred<0) so the **inner loop is guaranteed to reject and re-iterate**,
exercising the :412-417 shrink branch (including the actred<0 `temp`
formula :414), par growth, and eventually the accept branch; converges to
(1,1) hitting the info=1/2/3 cascade.
WRITE (labels, ES24.16 for reals, I4-style for ints):
`a_info, a_nliter, a_nfev, a_x1, a_x2, a_fnorm` (= enorm(2,Fvec) computed in
the driver), `a_fj11` (=Fjac(1,1)), `a_fj22`, `a_qtf1, a_qtf2, a_ipvt1,
a_ipvt2, a_diag1, a_diag2`.
Nliter/Nfev pin the FULL trajectory (any divergence in step acceptance
changes the counts); fj/qtf/diag pin the final factorization state.

**Case B — overdetermined exponential fit, Mode=2.**
`SUBROUTINE expfit`: for i=1..5, `Fvec(i)=X(1)*exp(X(2)*t(i))-y(i)` with
`t=(0.5,1.0,1.5,2.0,2.5)`, `y=(1.8,1.2,0.9,0.5,0.3)` (DATA in the
subroutine). Nonzero residual at the optimum, so the Ftol path terminates
with fnorm>0. Inputs: M=5, N=2, X0=(1.0, 0.0), Ftol=1e-10, Xtol=1e-10,
Gtol=1e-10, Mxiter=100, Epsfcn=0, **Mode=2, Diag=(2.0, 0.5)**, Factor=100,
Nprint=0, Nliter=0, Nfev=0, Ldfjac=5. Exercises M>N (pivoted rectangular
QR), the Mode=2 branches (:229 diag validation taken, :284/:347 rescale
SKIPPED — diag stays (2, .5)), and Gtol>0.
WRITE: `b_info, b_nliter, b_nfev, b_x1, b_x2, b_fnorm, b_fj11, b_fj22,
b_qtf1, b_qtf2, b_ipvt1, b_ipvt2`.

**Case C — cumulative counters + Info=5 (the Census-specific machinery).**
Reuse `rosen`. Inputs as Case A **except**: pre-set `Nliter=7, Nfev=13`
(fake prior accumulation, mimicking a later IGLS round), `Mxiter=10`
(= "Nliter+3", the rgarma.f:374 pattern). Expect exactly 3 accepted
iterations then Info=5 via :461 (`nliter>=mxiter` cumulative; maxfev
= max(10,200)*3 = 600 is not binding). Pins `begitr`/`oldfev` handling:
first-iteration scaling fires on `Nliter==begitr==7`, and final counters
must come back as oracle prints them (Nliter=10, Nfev=13+evals).
WRITE: `c_info, c_nliter, c_nfev, c_x1, c_x2, c_fnorm`.

Driver skeleton notes:
- `PROGRAM ref_lmdif`; `EXTERNAL rosen, expfit`; `LOGICAL F` `PARAMETER(F=.false.)`;
  pass `F,F` for Lauto/Gudrun. Arrays sized for the max case (fjac(5,2),
  wa4(5), fvec(5)).
- Argument order (:188-191):
  `CALL lmdif(rosen,2,2,x,fvec,F,F,ftol,xtol,gtol,mxiter,epsfcn,diag,mode,
  factor,0,info,nliter,nfev,fjac,2,ipvt,qtf,wa1,wa2,wa3,wa4)`.
- The driver may `INCLUDE 'error.cmn'` and set `Lfatal=.false.` for hygiene
  (with Nprint=0 it is never read; the include path already provides it to
  lmdif.f itself).
- Compute `a_fnorm` etc. via the linked `enorm(M,fvec)` (declare
  `DOUBLE PRECISION enorm / EXTERNAL enorm`).

**C++ test** (`tests/unit/test_numeric.cpp`, style of the existing
qrfac/lmpar/fdjac2 tests): three lambdas replicating rosen/expfit; call the
ported lmdif with identical inputs, `sync = {}`, `prtitr = {}`, nprint=0;
`CHECK_EQ` on info/nliter/nfev/ipvt, `rclose(..., 1e-12)` on x/fnorm/fj/qtf/
diag against the pasted oracle values. Case C must pre-set nliter=7, nfev=13
before the call. (Counts are exact-integer parity — they ARE the trajectory
canary.)

## 6. Parity risks specific to lmdif

1. **prered precedence (:401)**: `temp1**2 + temp2**2/P5` is
   `t1² + (t2²/0.5)` = `t1² + 2·t2²`. Misreading as `(t1²+t2²)/0.5` silently
   deforms ratio and the whole trust-region trajectory. dirder (:402) has NO
   /P5. Encode exactly as `temp1*temp1 + temp2*temp2/P5`.
2. **actred guard asymmetry (:386 vs :415)**: actred uses strict
   `P1*fnorm1 .lt. fnorm`; the step-bound clamp uses `.ge.` — together they
   are exhaustive but the boundary case must fall to `temp=P1`. Also actred is
   `-1` (not the formula) when the trial norm is ≥10× fnorm.
3. **Rejected-step `temp` (:413-415)**: two consecutive IFs (not IF/ELSE) —
   actred>=0 sets temp=P5, actred<0 sets the dirder formula; then the clamp
   possibly overwrites with P1. `par=par/temp` AFTER delta update; order
   matters since temp is reused.
4. **Info cascade ordering (:449-475)**: two groups, each ending in a
   `goto termination`. Within a group LATER assignments overwrite earlier
   (e.g. delta test overwrites info=1 with 2; gnorm<=epsmch overwrites 5/6/7
   with 8). Group 1 (1/2/3) runs BEFORE group 2 (5/6/8) — if info=1 fires,
   the mxiter test never runs this pass. Do not merge, reorder, or
   early-break inside a group.
5. **info=5 asymmetry**: :461 compares CUMULATIVE nliter to mxiter
   (mxiter=0 disables); :463 compares PER-CALL `nfev-oldfev` to
   `max(mxiter,200)*(n+1)`. Swapping these breaks the automdl/AIC paths that
   re-enter lmdif with accumulated counters (m3_scouting §6.4).
6. **Convergence tests run on REJECTED steps too**: the :449-475 cascade sits
   after the accept/reject branch, so a rejected step can still terminate
   (delta shrunk below xtol*xnorm ⇒ info=2 with the OLD x/xnorm — x is the
   last accepted point). Don't move the tests inside the accept branch.
7. **lckinv pattern**: false at :239 (initial eval) and :257 (inside fdjac2),
   **true** only at :378 (trial steps). This is where Census enforces
   invertibility (fcnar's penalty wall) — flipping any site changes the
   production trajectory even though the ref driver ignores the flag.
8. **delta clamp only on first iteration of THIS call** (:373):
   `nliter==begitr`, not `nliter==0`. Same for the diag/xnorm init block
   (:283). On re-entrant calls (Case C) these fire at nliter=begitr=7.
9. **qtf formation overwrites Fjac diagonal (:319)** with qrfac's rdiag (wa1)
   inside the same loop that applies the Householder reflections — keep the
   statement order (guard :309 tests the PRE-overwrite Fjac(j,j)).
10. **gnorm division placement (:332)**: `Fjac(i,j)*(Qtf(i)/fnorm)` — divide
    inside the accumulation, not `sum/fnorm` after. FP-order-sensitive.
11. **`-ffp-contract=off` discipline**: all `**2` become explicit `x*x`
    (matching qrfac port style); no std::pow, no fma-able rewrites.
12. **Nprint semantics**: Census REPLACED stock MINPACK's `fcn(iflag=0)`
    print convention with prtitr — there is no iflag=0 call to fcn in this
    lmdif. Do not reintroduce one; nfev counts are affected only by
    :241/:259/:379.
13. **Final prtitr guard reads `ratio` (:497)** — value from the last inner
    pass; guarded by `nliter>begitr` so init-to-zero is safe (§3).

## 7. Build command for the ref driver

lmdif.f references at link time: `fcn` (driver-supplied), `fdjac2`, `qrfac`,
`lmpar` (→`qrsolv`), `upespm`, `prtitr`, `dpmpar`, `enorm`, `dpeq`; at compile
time it INCLUDEs `error.cmn` (needs `-Ioracle/fortran`; provides only the
`/fcnerr/ Lfatal` COMMON — no init required). Minimal link set = the 8 real
MINPACK-side .f files + stubs for the two model-side routines **inside
ref_lmdif.f itself**:

```fortran
C     ---- model-side stubs: lmdif never reads back what these write ----
      SUBROUTINE upespm(Estprm)
      IMPLICIT NONE
      DOUBLE PRECISION Estprm(*)
      RETURN
      END

      SUBROUTINE prtitr(A,Na,Parms,Nparms,Itrlbl,Iter,Nfev)
      IMPLICIT NONE
      CHARACTER Itrlbl*(*)
      INTEGER Na,Nparms,Iter,Nfev
      DOUBLE PRECISION A(*),Parms(Nparms)
      RETURN
      END
```

(prtitr stub is link-only dead code at Nprint=0; upespm stub is called but is
a no-op, matching the C++ empty `sync`.)

Header comment / build (ref_minpack.f style; brace form for the header, and
the explicit PowerShell-safe expansion, from repo root):

```
C       gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_lmdif.f \
C         oracle/fortran/{lmdif,fdjac2,qrfac,qrsolv,lmpar,enorm,dpmpar,dpeq}.f \
C         -o ref_lmdif && ./ref_lmdif
```

PowerShell (no brace expansion):

```powershell
gfortran -O2 -ffp-contract=off -Ioracle/fortran tools/ref_lmdif.f oracle/fortran/lmdif.f oracle/fortran/fdjac2.f oracle/fortran/qrfac.f oracle/fortran/qrsolv.f oracle/fortran/lmpar.f oracle/fortran/enorm.f oracle/fortran/dpmpar.f oracle/fortran/dpeq.f -o ref_lmdif
./ref_lmdif
```

No other unresolved externals exist (verified against lmdif.f's call sites:
fcn :239/:257/:378 via fdjac2, upespm :265/:444, prtitr :272/:498, qrfac :278,
lmpar :359, dpmpar :212, enorm ×6, dpeq ×7).
