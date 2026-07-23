# User-defined regressor port map (`user=` / `data=` — regvar case 140)

**STATUS 2026-07-22: default + `usertype=` + `centeruser=` + `file=` DONE.**
5 gate specs, all green in test_m3_estimate (nreg=2, nefobs=143, loglik/aic +
MA coef match oracle .udg):
- `airline_user-reg.spc` (2 cols, default PRGTUD).
- `airline_user-reg-type.spc` (`usertype=(td seasonal)` → 2 groups).
- `airline_user-reg-cmean.spc` (`centeruser=mean`).
- `airline_user-reg-cseas.spc` (`centeruser=seasonal`).
- `airline_user-reg-file.spc` (`file=../data/userreg2.dat`, free-format).
Full parity 609 pass / 0 fail / 18 xfail. `user-reg{,-type,-cmean,-cseas}`
COMMITTED 6f15082; `-file` + the file=/format= wiring NOT yet committed.

`file=` (arg 5) → gtnmvc→rgfile; `format=` (arg 6) → rgfmt (formatted path
DEFERRED, inpter). Tail (getreg.f:558-567): hvfile && !haveux → gtfldt_free
reads the free-format matrix into ctx.arima.userx. Exposed gtfldt_free (was
anon-namespace in series.cpp; decl in specparse.hpp).

`usertype=` parse (arg 13): gtdcvc(URGDIC) → usrtyp per column; broadcast when
one type; havtd/havln/havlp/havhol set. Tail dispatches each usrtyp to the right
adrgef title. HOLIDAY types (holiday..holiday5) → inpter DEFERRED (need chkuhg).
`centeruser=` parse (arg 17): gtdcvc(URRDIC="meanseasonal") → lumean/luseas.
Tail centers Userx in place after the adrgef loop: mean = per-column mean over
all rows; seasonal = per-(season,column) mean (getreg.f:692-732).
Remaining: `usertype=holiday*` (chkuhg), `file=`/`format=` data source, fixed
user coefs (regfix/Userfx). Args still token-consumed.

Goal: gate `regression{ user=(...) data=(...) }` bit-exact vs oracle. The whole
`user=` parse + storage + builder is UNPORTED (usrreg_cmn exists but nobody
fills it; args 2/3/4/13/17 in `gt_regression` are token-consumed).

Fortran source: `oracle/fortran/getreg.f` (reader), `regvar.f` case 140 (builder).

## Minimal first gate
`user=(x1 x2) data=(...)` with DEFAULT type (no `usertype=`) → each column added
as a plain `PRGTUD` "User-defined" regressor group. Skips holiday-group
(`chkuhg`, unported) and outlier user types (`otsort`, unported). `centeruser`
default = off (no mean-centering).

## Storage — ALL SLOTS ALREADY EXIST
- `ctx.arima.userx`  — `farray1<double,53040>` (Userx, PUSERX). Layout: `ncusrx`
  consecutive values per observation row; element(row i 1-based, col j) =
  `userx[(i-1)*ncusrx + j]`.
- `ctx.arima.nrusrx` — int (Nrusrx = neltux/ncusrx).
- `ctx.arima.bgusrx` — `farray1<int,2>` (Bgusrx); default = Begsrs if no `start=`.
- `ctx.arima.luser`  — bool (aictest user flag).
- `ctx.usrreg.ncusrx` — int (# user columns, set by gtnmvc).
- `ctx.usrreg.usrtyp` — `farray1<int,52>` (PUREG), per-column type.
- `ctx.usrreg.usrptr` — `farray1lb<int,0,53>`, title-pointer array.
- `ctx.usrreg.usrttl` — `fstring<1196>` (packed column names).
- NEEDED locals (not stored globally): `neltux` (# data elements), `nusrrg`
  (# usertype entries — 0 for minimal gate). `nusrrg` has no ctx slot; add if
  usertype gate needed later.

## Helpers — PORTED (signatures)
- `gtnmvc(ctx, grpchr, flgnul, pelt, std::string& chrvec, ...)` — reads name
  list → packed string + ptrs + count. `specparse.hpp:200`. Used for `user=`.
- `gtdpvc(ctx, LPAREN, true, peltux, dvec, nelt, argok, inptok)` — reads double
  vector → Userx/neltux. Already used across readers_spec.
- `gtdtvc(ctx, havesp, sp, grpchr, flgnul, 1, Bgusrx, nelt, argok, inptok)` —
  reads a date → Bgusrx. `specparse.hpp:198`. Used for `start=`.
- `copycl(from, nr, nfrmcl, ifrmcl, ntocl, itocl, to)` — column copy,
  `ratpos.cpp:66`. Builder uses `from = &userx[ixymu*ncusrx]` (0-based),
  nfrmcl=ncusrx, ifrmcl=i (source col), ntocl=Ncxy, itocl=target col.
- `chkcvr(begdt, nrows, spandt, nspobs, sp)` — coverage check, `util.cpp`.
- `dfdate(d1, d2, sp, &out)` — month-count difference.
- `adrgef(ctx, initvl, effttl, grpttl, rtype, regfx, ...)` — add regression
  effect/group. `specparse.hpp:268`. Tail loop calls per column.
- `strinx(exact, dict, ptr, lo, hi, key)` — dictionary index.
- `getstr(ctx, ttl, ptr, n, i, out, &nchr)` — unpack i-th title.

## Helpers — UNPORTED (only for later, non-minimal types)
- `chkuhg` — user holiday-group sequence check. Skip (minimal = no holiday).
- `otsort` — sort user outlier regressors. Skip (minimal = no ao/ls/so user).

## Constants (`core/prm/gen/model.hpp`)
PUREG=52, PUSERX=53040.
PRGTUD=18 (plain "User-defined"), PRGTUS=38 (seasonal), PRGUCN=64 (constant),
PRGUTD=57 (TD), PRGULM=58, PRGULQ=59, PRGULY=60, PRGUAO=61, PRGULS=62,
PRGUSO=63, PRGUCY=65, PRGTUH=49, PRGUH2..5=50..53.
UTYDIC / utyptr (regvar.f:78) — the builder's type dictionary (15 entries,
"User-defined Seasonal" first). PUTY count. strinx-matched vs group title.

## Work items — STATUS
1. ✅ **gt_regression** (readers_spec.cpp): args 2/3/4 (user/data/start) wired to
   gtnmvc/gtdpvc/gtdtvc. usertype(13)/centeruser(17) still consumed (later).
2. ✅ **gt_regression tail** (getreg.f:571-736): validate hvuttl==haveux &
   neltux%ncusrx==0; default bgusrx=begsrs; nrusrx; chkcvr; loop adrgef(...,
   "User-defined", PRGTUD). usrtyp!=0 → inpter (typed path deferred). Skipped
   centeruser (lumean/luseas default off) + regfix/otsort (fixed/outlier paths).
3. ✅ **regvar case 140** (regvar.cpp): lckurg chkcvr guard, dfdate→ixymu,
   strinx(UTYDIC) type match, copycl loop. RESOLVED ⚠: strinx exact (chksub=F);
   bare "User-defined" matches NO dict entry → typidx=0 → itype=0 → the
   `itype==0` branch copies every column in order. Uses the regvar PARAMETERS
   userx/bgusrx/nrusrx (callers pass ctx.arima.*), no longer voided.
   BUILD GREEN + unit 10/10.
4. ⏳ **Gate**: synthetic 2-col data over airline span (144 rows, row-major
   interleaved: x1[t] x2[t] x1[t+1]...). Bless oracle → test_m3_estimate;
   remove "user=" (keep "usertype") from exclusion list test_m3_estimate.py.
   Estimate-only (no forecast) so nrusrx=144 covers begxy/nrxy.

## Gotchas
- `xrgmdl`/`usrxrg` are the SEPARATE x11reg store ([[x13cpp-current-status]]
  loadxr swap) — do NOT confuse ctx.usrreg (regARIMA) with ctx.usrxrg (x11reg).
- ixymu offset: user data may start before/after span; copycl source offset is
  `ixymu*ncusrx`. dfdate(Begxy, Bgusrx) gives ixymu.
- Builder runs per-group; case 140 hit once per user group. thisgp walks cols.
