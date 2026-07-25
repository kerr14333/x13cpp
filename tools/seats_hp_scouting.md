# SEATS Hodrick-Prescott trend/cycle — scope, what landed, what walls it

Covers `seats{ hpcycle= hplan= hptarget= hprmls= }`.

**Status:** the options→internal BRIDGE is ported and gated bit-exact. The HP
FILTER is **not** ported and is walled behind a large unported subsystem (the
SEATS forecast decomposition). This file is the port map for finishing it.

---

## 1. What the flags actually move (measured, not inferred)

Oracle blast-radius sweep (`x13as_ascii_O2.exe`, airline/payems/expgs,
`(0 1 1)(0 1 1)`, every output file diffed byte-for-byte flag-on vs flag-off):

| flag | files that move | max relative delta |
|---|---|---|
| `hplan=40` | `.cyc` `.ltt` `.sum` `.tbs` | cyc/ltt **5.10e-2** (airline), **5.63e-2** (payems), **3.08e-1** (expgs) |
| `hptarget=sadj` | `.cyc` `.ltt` `.tbs` | cyc **4.50e-2** / ltt 4.62e-5 (airline); cyc **3.73e-2** (payems); cyc **8.67e-2** (expgs) |
| `hptarget=orig` | `.cyc` `.ltt` `.tbs` | cyc **2.38e-1** / ltt 4.68e-3 (airline); cyc **4.08e-2** (payems); cyc **9.60e-2** (expgs) |
| `hpcycle=no` | `.sum` `.tbs`; **`.cyc`/`.ltt` disappear** | n/a (tables gone) |
| `hpcycle=yes` | *nothing* | 0 — it is already the default (`gtinpt.f:536` `Lhp=T`) |
| `hprmls=yes` | *nothing* on a regressor-free spec | 0; **with an LS outlier** (`ls1955.01`): cyc/ltt **1.44e-2** (airline), 4.39e-6 (payems) |

**The headline:** `s10`–`s18` — every table this port emits — are
**bit-identical (0.000e+00)** under every HP setting. The HP family is the one
`seats{}` option group that is genuinely inert with respect to the ported
decomposition. It is NOT an instance of "parsed, ignored, wrong numbers behind
`OUTCOME: OK`"; the numbers the port produces are right, the `cyc`/`ltt` tables
simply do not exist.

`hprmls` is inert unless `PaOutR` (the LS/ramp/trend-outlier regression effect,
`sigex.f:2421/2448/2480/2506`) is non-zero — i.e. it needs an LS or ramp
regressor to bite at all.

## 2. Option resolution — PORTED and GATED

`core/src/seats/seatopts.cpp`, `seats_resolve_options()` +
`seats_resolve_hpcycle()`. Faithful transcription of:

- `ansub9.f:1081-1090` — block 1: `Lhp` decides. `hplan` present ⇒ explicit
  `1` (or `Hptrgt`); absent ⇒ the `-1` "auto" sentinel. `Lhp` false ⇒ `0`.
- `ansub9.f:1109-1117` — block 2: `hplan=` sets `L_hplan` and re-enables a `0`.
  **This is CB-15** (`tools/census_bugs.md`): `hpcycle=no` is silently
  overridden by `hplan=`. Ported verbatim.
- `sigex.f:2370-2387` — the `-1` sentinel resolves at decomposition time on the
  series length (monthly needs `nz>=120`; 60/48/45/30/15 for `mq`=6/4/3/2/1),
  then targets `Hptrgt` if set, else `1`.
- `ansub9.f:1615/1049/1050` — `L_out` (default 0, `out=` override, then **any**
  seats PRINT table in `[LSETRN=349, NTBL-11=385]` forces 3). Note the `NTBL-11`
  upper bound excludes `LSECYC=388`/`LSELTT=389`, so `seats{print=(cyc ltt)}`
  keeps `out==0` while `seats{print=all}` does not — verified against the
  oracle.

`hpcycle` target codes: `1` = split the TREND, `2` = the SA series, `3` = the
ORIGINAL series (`sigex.f:2413/2434/2466`).

**Known gap in the `out` resolution:** `gt_seats` (readers_spec.cpp) token-
consumes `print=` without populating `ctx.tbllog.prttab` — the seats table
dictionary (DSEDIC, `seatpr.f`) is unported print surface — so the
`istrue(prttab, ...)` term is always false and `out` resolves to 0 for any spec
that does not say `out=` explicitly. That is why every existing corpus
`*_seats.spc` (all carry `seats{print=all}` ⇒ oracle `L_OUT==3`) has no `.cyc`
golden while the port reports `out==0`. `test_seats_hpopts.py` deliberately
does not gate `HPOPT_out`; the 7 hand-authored `*_hp-*-seats` specs omit
`print=` so that both sides agree at 0.

**Gates:** `tests/parity/test_seats_hpopts.py` (42 tests). Two independent
oracle cross-checks per spec:
1. the resolved `hpcycle`/`hplan` vs the oracle's own `.sum` SEATS "INPUT" echo
   (`ansub10.f:3613/4043`, which prints a value only when it differs from the
   namelist default — so every ordinary corpus spec asserts "resolves to the
   default");
2. whether the oracle wrote `.cyc`/`.ltt` at all (HPOUTPUT runs only at
   `hpcycle>=1 && out==0`), reached through a completely separate code path.

Both checks were mutation-tested: deleting the CB-15 re-enable fails 4 tests;
weakening the `nz>=120` span rule fails 1.

Corpus: `generated/{airline,payems}_hp-{hplan,off,relock}-seats` +
`generated/airline_hp-short-seats`. They also gate `s10`–`s18` in
`test_seats_tables.py` as a non-disturbance check (see the note there — that is
regression protection, *not* evidence HP is ported).

## 3. The HP filter — NOT ported. What it needs.

Call chain, `sigex.f:2388-2600` then `sigex.f:4021-4045`:

```
sigex.f:2392   HPPARAM(mq,hplan,HPper,HPpar,hpth,km,kc,g,h)      ansub10.f:4-118
sigex.f:2413   hpcycle==1: eTrend = Trend + Pareg(:,1) [+ PaOutR unless Lhprmls]
sigex.f:2434   hpcycle==2: extSA  = SA + Pareg(:,1,3,4,7) + PaOuIR [+ Pareg(:,5)] [+ PaOutR]
sigex.f:2466   hpcycle==3: extZ   = Z + Pareg(:,0..5,7) + PaOuIR + PaEast + PaTD + PaOuS [+ PaOutR]
sigex.f:2431   HPTRCOMP(<the above>, Nz, lfor, hptrend, hpcyc, hpth, km, g, h)  ansub10.f:203-305
sigex.f:2506   Lhprmls: hptrend += PaOutR   (put the LS effect back into the LONG-TERM TREND)
sigex.f:2517   compHP = hptrend + hpcyc  (exp() when lamd==0)
sigex.f:4038   HPOUTPUT(...)                                     ansub10.f:~1050-1365
                 -> USRENTRY 2501 = the `cyc` table, 2502 = `ltt`   ansub9.f:513-526
```

Leaf inventory:

| leaf | file:lines | notes |
|---|---|---|
| `HPPARAM` | ansub10.f:4-118 | λ↔period conversion, two complex roots via `SELROOT`, `CONVC`, one 3×4 `MLTSOL`. **`mltsol` is already ported** (`core/src/seats/factor.cpp:66`). Self-contained, low risk. |
| `SELROOT` | ansub10.f:120-148 | trivial (pick the smaller-modulus root) |
| `CONVC` | ansub10.f:150-200 | complex polynomial convolution, trivial |
| `HPTRCOMP` | ansub10.f:203-305 | two 4×5 `MLTSOL` solves + a forward and a backward 2-term recursion. Low risk **given** `extendHP`. |
| `extendHP` | ansub10.f:5418-5553 | **medium risk.** Extends the target series by `lf=4` at each end by temporarily substituting a synthetic `(0 2 2)` model into the SEATS COMMONs (`Q=2`, `TH=(-hpth2,-hpth3)`, `Qstar=2`, `P=BP=Pstar=BQ=0`, `nd=2`, `BPHIST=(2,-1)`, `INIT=2`) and calling `calcFx` + `Fcast` forward and time-reversed, centred on `wm` = the mean of the twice-differenced target. The C++ already has both halves — `calcfx_last_residuals` and `fcast_extend` in `core/src/seats/estbur.cpp` — but they are `static` in an anonymous namespace and take `SeatsModelOrders`/`SeatsCanonicalDenoms`, so they must be exposed and driven with a synthetic model. Verified by hand: `build_bphist` on `(p=0,bp=0,d=2,bd=0)` already yields exactly `bphist=[2,-1]`. Watch the `a(i)=a(i)/Detpri` scaling — estbur.cpp deliberately DROPS it for its own FCAST seeding (see the header comment there); confirm which convention `extendHP` needs. |
| `HPOUTPUT` | ansub10.f:~1050-1365 | assembles the two save tables. Gated on `out==0`. `ireg==0` branch only for the regressor-free case. `lamd==1`: `cyc = hpcyc`, `ltt = hptrend`. `lamd==0` (log): `cyc = 100*compHP/(kons*exp(hptrend))`, `ltt = kons*exp(hptrend)` — the `kons` constant must be sourced. |

### The wall

`HPTRCOMP` is handed the target over `1 .. Nz + lfor`, and `lfor` is
`max(Nfcst, max(8, 2*mq))` = **36** for a default monthly run
(`sigex.f:426`; empirically insensitive to `forecast{maxlead=}` — 0, 12 and 36
all produce identical `cyc`). The C++ SEATS produces **only the historical
span**: `estbur_historical` (see `core/src/seats/estbur.hpp`, "SCOPE") returns
`trend/sa/sc/cycle` of length `Nz`. The forecast decomposition —
`ansub3.f:356-678`, the `npsi!=1` / `Nchi!=1` / cycle FORECAST blocks — is
explicitly unported.

Those 36 points are **not** negligible. For the default λ (`hpPer = 10*mq`
⇒ λ ≈ 1.33e5 monthly) the `hpth` roots have modulus ≈ 0.9637, so the tail
boundary term still carries `0.9637^36 ≈ 0.26` weight at the last observation.
The `hptrend` values over the historical span genuinely depend on the forecast
tail.

**Consequence: the HP filter cannot be ported bit-exact until the SEATS
forecast decomposition is ported.** That is a separate, independently gateable
deliverable — the oracle ships `tfd`/`sfd`/`ofd`/`afd`/`yfd` goldens (trend /
seasonal / series / SA / transitory forecast decomposition) for the whole
existing SEATS corpus, so `ansub3.f:356-678` can be closed and gated on its own
before anyone touches HP.

### Recommended order

1. Port `ansub3.f:356-678` (SEATS forecast decomposition). Gate on
   `tfd`/`sfd`/`ofd`/`afd` against the existing corpus goldens.
2. Expose `calcfx_last_residuals` / `fcast_extend` from `estbur.cpp`; port
   `extendHP`. No direct gate — validate indirectly via step 3.
3. Port `HPPARAM` + `SELROOT` + `CONVC` + `HPTRCOMP` + `HPOUTPUT`'s `ireg==0`
   branch. Gate `cyc`/`ltt` against
   `generated/{airline,payems}_hp-hplan-seats` and
   `generated/{airline,payems}_hp-relock-seats` (goldens already blessed and in
   the repo). Delete the "the harness now emits cyc/ltt" tripwire assertion at
   the end of `test_seats_hpopts.py`.
4. `hptarget=sadj`/`orig` (`hpcycle` 2/3) additionally need `Pareg(:,0..7)`,
   `PaOuIR`, `PaEast`, `PaTD`, `PaOuS` — the per-effect deterministic component
   arrays. Only `hpcycle==1` (the default target) is reachable without them.
5. `hprmls` is three lines once (3) lands, but needs an LS/ramp regressor spec
   to be non-vacuous — `regression{variables=(ls1955.01)}` moves cyc/ltt by
   1.44e-2 on airline.

## 4. Not in scope here

`out=`/`printphtrf` are print surface (deferred by design).
`noadmiss`/`imean`/`statseas`/`bias`/`finite` are separate option-family items.
