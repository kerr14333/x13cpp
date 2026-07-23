# Codex review of a5dffc8 (x11easter)

**Findings**

1. [core/src/driver/run_x11.cpp](D:/code_projects/x13new/core/src/driver/run_x11.cpp:66) - BUG - Easter factors are generated with the pre-pass forecast/backcast pointers still forced to the observed span.
   
   C++ sets `Nfcst=Nbcst=0`, `Pos1bk=Pos1ob`, `Posffc=Posfob` at lines 45-51, then calls `holday(..., iforc=0, xdsp=0)` at line 66, and only restores the saved extension/pointer state at lines 68-74. The oracle does the opposite: `xrgdrv.f` forces zero forecast/backcast for the transparent X-11 pass at lines 134-146, but restores `Nfcst/Nbcst/Nfdrp`, `Pos1bk=Pos1ob-Nbcst`, `Posffc=Posfob+Nfcst`, `Nofpob`, and `Nbfpob` at lines 168-179 before `CALL holday(Sti,...,Nfcst,Xdsp)` at lines 183-184. `holday.f` then uses `Pos1bk` and `Iforc` to initialize/generate the holiday span at lines 33-50.
   
   This is latent for the no-forecast/no-backcast gate, but diverges when backcasts are present and when forecasts exceed the default 12-month fallback. Backcast Easter factors are generated from the observed start in C++, not `Pos1bk`, and long forecast factors beyond 12 periods remain identity.
   
   Suggested fix: after the transparent `x11pt1/2/3`, restore `Nfcst/Nbcst/Nfdrp/Nofpob/Nbfpob/Pos1bk/Posffc` before calling `holday`, and pass the restored `nfcst0` plus the correct `Xdsp` equivalent rather than hard-coded zero.

2. [core/src/x11/x11parts.cpp](D:/code_projects/x13new/core/src/x11/x11parts.cpp:372) - BUG - Dropping the `Khol==2` x11pt2 holiday combine is not purely deferred-output state.
   
   The C++ removed `opt.khol >= 2` from the fatal gate but does not implement the corresponding `Fachol += X11hol` work. Oracle `x11pt2.f` counts `Khol.eq.2` as a holiday factor at lines 297-301 and executes `CALL addmul(Fachol,Fachol,X11hol,Pos1bk,lsthol)` at line 309. Later, oracle `x11pt3.f` uses that non-output state: when `Khol.eq.2`, it executes `CALL divsub(Faccal,Faccal,Fachol,Pos1bk,klda)` at lines 525-540. The C++ has the same later divide at [x11parts.cpp](D:/code_projects/x13new/core/src/x11/x11parts.cpp:792), but `Fachol` is still identity from `x11int` ([x11drv.cpp](D:/code_projects/x13new/core/src/x11/x11drv.cpp:122)).
   
   This is harmless in the simple x11easter-only D10-D13 gate because no later calendar-final combine consumes `Faccal`. It becomes wrong with X-11 Easter plus another final calendar factor such as regARIMA TD: Fortran removes `X11hol` from `Faccal` via `Fachol`; C++ leaves it in `Faccal`, so the later D11/D16 calendar combine can double-remove Easter.
   
   Suggested fix: port the non-output part of `x11pt2.f:297-309`: compute `lsthol` and `addmul(Fachol, Fachol, X11hol, Pos1bk, lsthol)` when `Khol==2`, while continuing to defer only the table/punch output.

3. [core/src/driver/run_x11.cpp](D:/code_projects/x13new/core/src/driver/run_x11.cpp:205) - BUG - The editor rejection rules for X-11 Easter are incomplete.
   
   C++ enables Easter for `keastr >= 1 && !haveum`, then only rejects quarterly data at lines 205-217. Oracle `editor.f` does more in the same `Khol` block: after setting `Lgenx=T; Khol=1`, it rejects quarterly `Sp.eq.4` at lines 1921-1925, rejects automatic transform selection `Fcntyp.eq.0` at lines 1926-1932, rejects non-multiplicative X-11 modes `Muladd.gt.0` at lines 1933-1939, rejects conflicting regARIMA Easter holiday adjustment at lines 1941-1952, rejects conflicting irregular-regression Easter at lines 1954-1960, and rejects spans beginning before 1901 at lines 1961-1968.
   
   This means specs such as `x11{mode=add x11easter=yes}` or `transform{function=auto} x11{x11easter=yes}` can proceed in C++ even though the oracle aborts before running the adjustment.
   
   Suggested fix: mirror the full `editor.f:1926-1968` validation before `x11_easter_prepass`, at minimum `fcntyp != 0`, `muladd == 0`, no conflicting Easter holiday source, and `Begspn(1) >= 1901`.

4. [core/src/x11/x11easter.cpp](D:/code_projects/x13new/core/src/x11/x11easter.cpp:422) - FAITHFULNESS-GAP - The inadmissible-Easter abort mutates only a local `ihol`, not `ctx.x11opt.keastr`.
   
   Oracle `holidy.f` passes `Keastr` by reference into `easter` at line 71, and `easter.f` sets `Ihol=0` and returns when any `Ieast` bin is zero at lines 31-35. Since the actual argument is `Keastr`, the global `Keastr` is cleared. C++ creates `int ihol = keastr` at line 423 and passes that local to `easter`; if the same bin-zero path fires, [x11easter.cpp](D:/code_projects/x13new/core/src/x11/x11easter.cpp:88) sets only the local and returns.
   
   Current main D10-D13 output may still look like the oracle because `X11hol` remains identity and `Khol` is set to 2 in both implementations, but any later code inspecting `Keastr` will see a stale enabled value in C++.
   
   Suggested fix: pass `ctx.x11opt.keastr` by reference through `holidy/easter`, or write the mutated `ihol` back after the call.

I did not find a substantive mismatch in the direct `chkeas`/`easter` label 10/20/30/40 control flow, the `mfreq` table, or the visible `kdate` values while comparing them line by line.
