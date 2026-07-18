# Generated corpus

Systematically generated X-13ARIMA-SEATS spec files for parity testing.
**Do not edit the `*.spc` files by hand** — regenerate them:

```
python genspecs.py
```

The generator is deterministic and idempotent: every run wipes and rewrites all
`*.spc` files and `MANIFEST` here, and touches nothing outside this directory.

## What it crosses

**4 base series** × **8 configurations** = **32 specs**.

Series (from `../data/`): `airline`, `payems`, `unrate` (a rate → **no
transform** ever, and a `span` to stay within the program's 780-obs limit),
`expgs` (quarterly).

Configurations:

| config                  | model / adjustment |
|-------------------------|--------------------|
| `x11-default`           | X-11, multiplicative (additive for the rate series) |
| `x11-logadd`            | X-11, log-additive mode |
| `x11-additive`          | X-11, additive mode |
| `seats`                 | `automdl` + SEATS |
| `automdl-x11`           | `automdl` + forecast + X-11 |
| `fixed-airline-x11`     | `(0 1 1)(0 1 1)` + td regression + outlier + forecast + X-11 |
| `fixed-airline-seats`   | `(0 1 1)(0 1 1)` + forecast + SEATS |
| `automdl-aictest-x11`   | `automdl` + `aictest` td/easter regression + X-11 |

## Output decoration

Each spec is decorated to exercise as much output as possible:

- `print = all` on every spec that supports a print argument.
- `save = (...)` with the **complete** list of savable tables for each spec.
- `savelog = all` on every spec where that is legal.

### Why explicit `save = (...)` and not `save = all`

In X-13ARIMA-SEATS v1.1 b61, `save = all` is **not** valid: unlike `print`
(which accepts the level keywords `none/brief/default/all/alltables`), the
`save` argument only accepts explicit table names — the parser looks each token
up in the spec's save-table dictionary and has no `all` level
(`oracle/fortran/getsav.f`). So `genspecs.py` emits the full explicit list of
savable tables per spec instead. Those lists were extracted directly from the
save-table dictionary in `oracle/fortran/stable.prm` / `stable.var`.

`savelog = all`, by contrast, *is* valid — but only in the specs whose savelog
dictionary contains the `alldiagnostics`/`all` entry (`automdl`, `estimate`,
`check`, `x11`, `seats`, `history`, `spectrum`, `composite`). The generator adds
`savelog = all` only there; specs like `transform`, `regression` and `outlier`
have savelog diagnostics but no `all` shortcut, so it is omitted for them.

Rule for x11 mode: when a `transform{}` spec is present the x11 `mode` is left
unset so the program harmonises it with the transform; when there is no
transform, the mode is set explicitly.

## Files

- `genspecs.py` — the generator (single source of truth).
- `MANIFEST` — inventory of generated specs (regenerated each run).
- `*.spc` — the generated specs, named `<series>_<config>.spc`.
