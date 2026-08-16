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

**22 walls.**


## Automatic model selection

- **`core/src/automdl/aictst.cpp:1303`**
  regression{chi2test=yes} with user-defined holiday regressors -- chkchi.f is not ported

- **`core/src/automdl/automd.cpp:77`**
  automdl{} with regression{aictest=} naming anything other than td, easter or user -- td1coef / tdstock / lom have no automd path

- **`core/src/automdl/automx.cpp:469`**
  pickmdl{} with user-defined holiday chi-square testing is not yet ported (chkchi.f has no C++; automx.f:484-500 runs it inside the candidate loop). The user-regressor half of this wall came down when usraic.f was ported.
  *Fortran:* `automx.f:484-500`


## Composite

- **`core/src/composite/agr3.cpp:354`**
  agr3 sliding-spans store of the FORCED indirect SA (agr3.f:497's ssrit)
  *Fortran:* `agr3.f:497`

- **`core/src/composite/agr3.cpp:383`**
  agr3 sliding-spans store of the ROUNDED indirect SA (agr3.f:540's ssrit)
  *Fortran:* `agr3.f:540`


## Diagnostics

- **`core/src/diag/amdfct.cpp:195`**
  out-of-sample BACKCASTS with an outlier regressor inside the first three years are not yet ported exactly (amdfct.f:92-148 under Bckcst): measured 6.6959 against the oracle's 6.71.
  *Fortran:* `amdfct.f:92-148`


## Drivers

- **`core/src/driver/run_seats.cpp:144`**
  transform{constant=} through SEATS (seatpr.f:211-390 constant removal)
  *Fortran:* `seatpr.f:211-390`

- **`core/src/driver/run_seats.cpp:394`**
  SEATS historical-span decomposition (ESTBUR general-branch solve) -- either an unsupported model shape (p>0/bp>0/imean!=0) or the chain failed; see tools/seats_scope.md


## regARIMA

- **`core/src/regarima/regvar.cpp:312`**
  change-of-regime base type

- **`core/src/regarima/regvar.cpp:315`**
  regression variable type


## Spec parser

- **`core/src/specparse/getreg_vars.cpp:612`**
  change-of-regime trigonometric seasonal regressors (adrgim.f)

- **`core/src/specparse/getreg_vars.cpp:1142`**
  AOS/LSS outlier regressors (rdotls.f)


## X-11

- **`core/src/x11/slidingspans.cpp:793`**
  slidingspans{} with a change-of-regime regression variable (ssmdl.f:150-241, which the oracle itself halts on -- see CB-39) is
  *Fortran:* `ssmdl.f:150-241`

- **`core/src/x11/x11parts.cpp:232`**
  x11pt1 prior trading-day adjustment (pritd/ssrit)

- **`core/src/x11/x11parts.cpp:283`**
  x11pt1 additive/pseudo-additive prior trading-day

- **`core/src/x11/x11parts.cpp:566`**
  x11pt2 x11reg factor combine+emit

- **`core/src/x11/x11reg.cpp:347`**
  x11regression{holidaynonlin=yes} -- the Bell-Hilmer nonlinear Easter (rgtdhl.f / kfcn.f / estrmu.f)

- **`core/src/x11/x11reg.cpp:428`**
  x11ref forcecal= combined calendar factor

- **`core/src/x11/x11reg.cpp:1028`**
  x11aic user branch with umdata= (Haveum)

- **`core/src/x11/x11reg.cpp:1874`**
  x11mdl Kswv=3 with no Trading Day group

- **`core/src/x11/x11reg.cpp:1891`**
  x11mdl Kswv=3 forcecal= combine

- **`core/src/x11/xrgdrv.cpp:57`**
  xrgdrv OLS prior trading-day (Ixreg>=2) for an additive/pseudo-additive adjustment or a classic X-11 Easter (Khol==1)


# Faithful refusals -- the oracle declines too

Not gaps. The oracle rejects the same input, so refusing IS the
port. Listed so the count above is not mistaken for the whole
inventory.

**4 walls.**


## Automatic model selection

- **`core/src/automdl/automx.cpp:491`**
  Must have user supplied models stored in

- **`core/src/automdl/automx.cpp:525`**
  No ARIMA models stored in

- **`core/src/automdl/automx.cpp:534`**
  Every pickmdl candidate model failed to estimate.

- **`core/src/automdl/automx.cpp:813`**
  pickmdl{}: the selected model failed to re-estimate.


---

*Classification is by keyword on the message text (`GAP_RE` in
`tools/walls.py`) -- a heuristic. If a wall is filed under the
wrong heading, fix its MESSAGE: that string is what a user
actually sees when a run stops.*
