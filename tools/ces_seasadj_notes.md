## Triage of the full production set (2026-07-24)

Reproduce with the two committed tools:

```
# inputs (contact UA required -- see above)
curl -A "x13cpp-research <you>" -O https://www.bls.gov/web/empsit/ces.spec.ae.zip
curl -A "x13cpp-research <you>" -O https://www.bls.gov/web/empsit/ces.spec.other.zip
# one ce.data.<NN>a.<name>.Employment per supersector (~75 MB total; the exact
# names come from the directory listing -- they end in `.Employment`, and a
# truncated name 404s)
curl -A "x13cpp-research <you>" -O https://download.bls.gov/pub/time.series/ce/ce.data.10a.MiningAndLogging.Employment

python tools/ces_make_corpus.py --specs <ae dir> --regressors <other dir>     --ce-data <data dir> --out <corpus>
python tools/ces_triage.py --corpus <corpus>     --oracle oracle/fortran/x13as_ascii_O2.exe --engine build/x13run_x11.exe     --composite build/x13run_composite.exe --jobs 10 --json triage.json
```

**Set:** 142 specs = 78 direct (`AE*`) + 14 composite totals (`CO*` with
`composite{}`) + 50 components (`CO*` with `comptype=`) -> 89 runnable jobs
(components run inside their group's metafile).

**Result: 86/89 at 1e-12, 88/89 at the 1e-6 estimation floor.**

| verdict | n | notes |
|---------|---|-------|
| bit-exact (<=1e-12) | 86 | incl. 10 of 11 composite groups: d10/d11 direct + isf/isa indirect |
| estimation-floor    | 2  | `CO6562100000` isf 5.9e-7; `AE4348100000` d10 4.1e-8 |
| outlier             | 1  | `AE7072200000` -- see below |
| not runnable        | 3  | `CO2023610000/620000/800000`: components live in the 667-spec `ces.spec.ae2.zip`, not in `ces.spec.ae.zip`, so the ORACLE fatals too (its own SIGFPE). Not a port issue. |

**`AE7072200000` (Food services and drinking places)** -- the one genuine
frontier case. d11 2.55e-6, d16 6.9e-3, d10 1.73e-1. The d10 headline is
inflated: `mode=add` centres the seasonal factors on zero, so the max ABSOLUTE
d10 error (2.27e-2 on a series of scale ~389) lands at a near-zero factor. Still,
d11 at 2.55e-6 is just outside the estimation floor. Not a TC-regressor problem:
49 of the 50 TC-carrying specs are bit-exact. Distinctive features are 18 manual
outlier `variables=` (a dense COVID LS/TC cluster) on a `(0 1 2)(0 1 1)` model
with user TD -- i.e. a hard optimisation surface. Estimation-path, not structural.

**Two real port bugs this triage surfaced** (both fixed, commit 8f0a785):
1. `composite{}` never set `Havesp` (getcmp.f:213) -- the CES totals have no
   `series{}` spec, so `x11{seasonalma=}` fatalled. All 11 groups were dead.
2. `Orig2`'s forecast region was never populated (arima.f:1433-1441). Orig2 is
   what composite adjustment aggregates, so O2/O5 -- hence the indirect seasonal
   factors -- were built from zeros over the appended forecast span. The
   census-examples corpus could not see it (no `forecast{}` there); every CES
   spec has `forecast{maxlead=24} + appendfcst=yes`.

## Disk


`ces_series/` (337 MB, mostly `ce.data.0.AllCESSeries`) was **deleted 2026-07-24**
on the user's say-so — only ~143 tiny NSA series are ever needed and those already
live in `tests/corpus/ces/data/`. Re-fetch with the curl + contact-User-Agent
recipe above when the spec generator needs the bulk file again (`ce.series`, the
3.8 MB series-id map, comes back the same way).

## Vertical slice kept as the canonical example

`tests/corpus/ces/AE1011330000_simple.spc` (stripped, uppercase BLS keywords) +
`data/AE1011330000.dat` + golden under `tests/golden/ces/AE1011330000_simple/` is
**committed as the uppercase parser-fidelity gate** — bit-exact a1/d10/d11. The full
BLS spec `AE1011330000.spc` (USER TD regressors + `FINAL=USER`) is kept WIP
(uncommitted) pending the FINAL=USER port (open item 1 above); it parses + runs
OUTCOME OK and its a1 is bit-exact, but D11 is not (FINAL=USER).
