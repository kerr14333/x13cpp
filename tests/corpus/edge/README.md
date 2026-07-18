# Edge-case specs

Hand-written specs that probe boundary conditions and error handling. Unlike the
`generated/` corpus, these are deliberately special. Run each from this
directory.

| file                         | probes |
|------------------------------|--------|
| `long780.spc` + `long780.dat`| A series at **exactly** the program's maximum monthly length, `POBS = 65*12 = 780` obs (`oracle/fortran/srslen.prm`). Boundary parity. |
| `malformed-unknown-arg.spc`  | An **unknown argument** (`frobnicate`) in the series spec. **Expected to fail** — the test is that the C++ port's error output matches the Fortran oracle's exactly. |
| `span-modelspan.spc`         | `span` (analysis window) + `modelspan` (narrower model-estimation window) subsetting on PAYEMS, with automdl + forecast + X-11. |

## `long780.dat`

Synthetic, deterministic monthly series (trend + seasonal + gentle wiggle,
positive integers), 780 observations, 1950.01–2014.12. Generated solely to hit
the length boundary; it is committed as a frozen part of the corpus.

## Note on the over-limit case

A series *exceeding* 780 monthly obs is also covered by the corpus: `unrate.dat`
(932 obs full history) — see `../data/unrate.README.md`. The generated UNRATE
specs apply a `span` to stay within the limit; feeding the whole series without
a span would exceed `POBS`.
