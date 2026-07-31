# Where the engine declines to run

**Generated -- do not edit.** `python tools/walls.py --write`

This port's rule is that an unported branch **fatals with a message naming the
Fortran line**, never silently does the default thing. The characteristic defect
in this codebase is an option the parser accepts and the engine ignores --
`OUTCOME: OK` with wrong numbers -- and a wall is what makes that impossible.

So this list is the honest answer to *"what is not ported yet"*, derived from the
source rather than from anybody's memory. If a wall is here, a spec that reaches
it stops with this message. If a feature is NOT here and not gated by a test,
that is the dangerous case and worth a look.

Not included, deliberately: ~90 bare `abend(ctx)` calls, which mostly reproduce
the Fortran's own error exits -- the oracle refuses those inputs too, so they are
ports rather than gaps.



# Gaps -- the oracle does this, the port declines

The honest to-do list. Each names the Fortran it would have to
reproduce.

**17 walls.**


## Automatic model selection

- **`core/src/automdl/automx.cpp:458`**
  pickmdl{} with regression{aictest=(user)} or user-defined holiday chi-square testing is not yet ported (usraic.f / chkchi.f have no C++; automx.f:463-500 runs them inside the candidate loop).
  *Fortran:* `automx.f:463-500`

- **`core/src/automdl/automx.cpp:555`**
  pickmdl{}: a trading-day AIC verdict that DIFFERS between candidates (automx.f:259-296's Picktd restore) is not yet bit-exact -- d11/d13 land 0.885%/2.655% off on Februaries.
  *Fortran:* `automx.f:259-296`


## Diagnostics

- **`core/src/diag/amdfct.cpp:195`**
  out-of-sample BACKCASTS with an outlier regressor inside the first three years are not yet ported exactly (amdfct.f:92-148 under Bckcst): measured 6.6959 against the oracle's 6.71.
  *Fortran:* `amdfct.f:92-148`


## Drivers

- **`core/src/driver/run_seats.cpp:141`**
  transform{constant=} through SEATS (seatpr.f:211-390 constant removal)
  *Fortran:* `seatpr.f:211-390`

- **`core/src/driver/run_seats.cpp:370`**
  SEATS historical-span decomposition (ESTBUR general-branch solve) -- either an unsupported model shape (p>0/bp>0/imean!=0) or the chain failed; see tools/seats_scope.md


## regARIMA

- **`core/src/regarima/regvar.cpp:312`**
  change-of-regime base type

- **`core/src/regarima/regvar.cpp:315`**
  regression variable type


## Spec parser

- **`core/src/specparse/getreg_vars.cpp:591`**
  change-of-regime trigonometric seasonal regressors (adrgim.f)

- **`core/src/specparse/getreg_vars.cpp:1121`**
  AOS/LSS outlier regressors (rdotls.f)


## X-11

- **`core/src/x11/x11parts.cpp:225`**
  x11pt1 prior trading-day adjustment (pritd/ssrit)

- **`core/src/x11/x11parts.cpp:261`**
  x11pt1 additive/pseudo-additive prior trading-day

- **`core/src/x11/x11parts.cpp:527`**
  x11pt2 user/seasonal/cycle/x11reg factor combine+emit

- **`core/src/x11/x11parts.cpp:944`**
  x11pt3 revisions seasonal store (getrev)

- **`core/src/x11/x11parts.cpp:1160`**
  x11pt3 revisions SA store (getrev)

- **`core/src/x11/x11parts.cpp:1270`**
  x11pt3 revisions forced-SA store (getrev)

- **`core/src/x11/x11parts.cpp:1358`**
  x11pt3 revisions trend store (getrev)

- **`core/src/x11/xrgdrv.cpp:44`**
  xrgdrv OLS prior trading-day (Ixreg>=2): only the TD-only multiplicative path is ported


# Faithful refusals -- the oracle declines too

Not gaps. The oracle rejects the same input, so refusing IS the
port. Listed so the count above is not mistaken for the whole
inventory.

**4 walls.**


## Automatic model selection

- **`core/src/automdl/automx.cpp:480`**
  Must have user supplied models stored in

- **`core/src/automdl/automx.cpp:514`**
  No ARIMA models stored in

- **`core/src/automdl/automx.cpp:523`**
  Every pickmdl candidate model failed to estimate.

- **`core/src/automdl/automx.cpp:806`**
  pickmdl{}: the selected model failed to re-estimate.


---

*Classification is by keyword on the message text (`GAP_RE` in
`tools/walls.py`) -- a heuristic. If a wall is filed under the
wrong heading, fix its MESSAGE: that string is what a user
actually sees when a run stops.*
