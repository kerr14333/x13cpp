# CES probe series — provenance

Three NSA series from the BLS Current Employment Statistics program, added
2026-07-28 as **automdl model-selection probes**. They are here because the
existing corpus series cannot exercise `automdl{}`'s thresholds: those knobs
adjust unit-root and cancellation limits by amounts smaller than the BIC gap
between airline's top two candidates.

| file | series id | what | n | start |
|---|---|---|---:|---|
| `ces_amuse.dat` | `CEU7071300001` | Amusements, gambling, and recreation | 360 | 1990.01 |
| `ces_leis.dat` | `CEU7000000001` | Leisure and hospitality (supersector) | 360 | 1990.01 |
| `ces_accfood.dat` | `CEU7072000001` | Accommodation and food services | 360 | 1990.01 |

All monthly, all-employees, **not seasonally adjusted** (the `CEU` prefix; the
`CES` prefix is the adjusted counterpart and has nothing left to find).

Source: `https://download.bls.gov/pub/time.series/ce/ce.data.70a.LeisureAndHospitality.Employment`
(BLS requires a contact User-Agent; `curl -A "x13cpp-research <email>"` works.)

## Why these three, measured

Oracle, `automdl{} + x11{}` under `transform{function=log}`:

| series | qsori | f3.m07 | bic2−bic1 | top two candidates |
|---|---:|---:|---:|---|
| `ces_amuse` | 620.8 | 0.075 | **0.001** | `(1 1 1)(1 1 1)` / `(0 1 0)(1 1 1)` |
| `ces_leis` | 626.6 | 0.093 | **0.002** | `(1 1 1)(1 1 1)` / `(0 1 0)(1 1 1)` |
| `ces_accfood` | 612.0 | 0.106 | **0.005** | `(1 1 1)(1 1 1)` / `(1 1 1)(0 1 1)` |
| *airline (reference)* | 167.6 | 0.202 | 0.014 | |

Two criteria at once, and both are necessary:

* **Seasonality** — `qsori` ~615 against airline's 168, `M7` well under 1.
* **Near-tied candidates** — a gap of 0.001–0.005 BIC, so a threshold nudge can
  actually flip the choice.

`ces_accfood` earns its place separately: its tie is in the **seasonal AR**
order, which no other corpus series offers, and that is what `ub2` / `urfinal` /
`seasonaloverdiff` move.

## Two deliberate choices

1. **The span stops at 2019.12.** The COVID collapse is a level shift an order
   of magnitude larger than the seasonal amplitude; including it would drive
   the model choice through outlier handling rather than through the thresholds
   these series exist to probe. A COVID-spanning variant would be a good
   *outlier* corpus addition — it is not this one.
2. **`unrate` / `payems` / `expgs` were rejected as probes** despite `unrate`
   having a 0.001 gap of its own: all three measure `qsori ≈ 0` with `M7 > 1`,
   i.e. **no identifiable seasonality**, so their near-ties are between
   nonseasonal candidates and cannot exercise a seasonal threshold at all.
