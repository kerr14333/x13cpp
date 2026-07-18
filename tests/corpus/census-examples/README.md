# Census manual examples

Canonical example specs from the Census Bureau *X-13ARIMA-SEATS Reference
Manual* ("Getting Started" chapter), adapted to reference this repo's corpus
data with relative paths.

## Provenance

The Census distribution ships documentation as PDFs only
(`https://www2.census.gov/software/x-13arima-seats/x13as/unix-linux/documentation/`
contains `docx13as.pdf`, `docx13ashtml.pdf`, and the quick-reference PDFs — no
downloadable `.spc` sample set, and the ASCII-source tarball contains no example
specs). These specs are therefore **hand-transcribed** from the manual's worked
examples rather than downloaded, kept faithful to the manual's wording and
argument choices. Checked against the vendored Fortran source
(`oracle/fortran/`) for argument validity. Transcribed 2026-07-18.

## Examples

| file                              | demonstrates |
|-----------------------------------|--------------|
| `01-basic-x11.spc`                | Minimal X-11 run, all defaults (airline series). |
| `02-airline-log-td-easter.spc`    | Log transform, `td` + `easter[8]` regression, `(0 1 1)(0 1 1)` airline model, X-11. |
| `03-automdl.spc`                  | Automatic transform + automatic ARIMA identification (`automdl`), X-11. |
| `04-seats.spc`                    | Log + airline model + **SEATS** adjustment (`seats{}`; mutually exclusive with `x11{}`). |
| `composite/`                      | Composite (indirect) adjustment via a metafile of component specs. |

## Running a single spec

X-13 resolves `file=` relative to the current working directory, so run each
spec **from this directory**:

```
x13as -i 01-basic-x11 -o /tmp/01-basic-x11
```

(The `-i` name is the spec basename without the `.spc` extension.)

## Running the composite example

The composite example is a **metafile** run: two component series
(`region_north`, `region_south`) plus a `total` spec whose `composite{}` spec
aggregates them. The two component data files (`region_north.dat`,
`region_south.dat`, 180 monthly obs, 1990.01–2004.12) are small deterministic
synthetic series generated solely to exercise the composite code path.

```
cd composite
x13as -m composite            # -m = metafile mode; reads composite.mta
```

`composite.mta` lists the spec basenames in processing order; the last spec
(`total`) must be the one carrying the `composite` spec.
