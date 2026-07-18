# unrate.dat — US unemployment rate (FRED UNRATE)

- **Series:** Unemployment Rate (`UNRATE`).
- **Source:** U.S. Bureau of Labor Statistics via FRED, St. Louis Fed.
  Canonical URL: `https://fred.stlouisfed.org/graph/fredgraph.csv?id=UNRATE`
- **Access:** Retrieved from the Internet Archive Wayback Machine capture
  `20250927033136` (byte-identical to the FRED CSV; direct FRED access was
  blocked from the build environment). See `fetch_fred.py`.
- **Download date:** 2026-07-18.
- **Units:** Percent.
- **Frequency:** Monthly.
- **Span (this file):** 1948.01 – 2025.08 (932 observations) — **full history.**
- **Format:** X-13 free format, one value per line.

## Important: exceeds the program's 780-obs monthly limit

This build supports at most `POBS = 780` monthly observations
(`oracle/fortran/srslen.prm`, `PYR1*PSP = 65*12`). This full-history file has
932 observations, so a **default** run on the whole series would overflow the
analysis span. Specs that analyze UNRATE therefore apply a `span` to restrict
the analyzed window to ≤ 780 observations, e.g.:

```
series{
  title  = "US Unemployment Rate (UNRATE)"
  file   = "../data/unrate.dat"
  start  = 1948.01
  period = 12
  span   = (1961.01, )     # 1961.01–2025.08 = 776 obs (<= 780)
}
```

Because it is a rate (already a percentage), UNRATE specs use **no transform**
(`transform{ function = none }`).
