# expgs.dat — US exports of goods and services (FRED EXPGS)

- **Series:** Exports of Goods and Services (`EXPGS`).
- **Source:** U.S. Bureau of Economic Analysis (NIPA) via FRED, St. Louis Fed.
  Canonical URL: `https://fred.stlouisfed.org/graph/fredgraph.csv?id=EXPGS`
- **Access:** Retrieved from the Internet Archive Wayback Machine capture
  `20260718181652` (byte-identical to the FRED CSV; direct FRED access was
  blocked from the build environment, so the capture was created on the download
  date via the Wayback "Save Page Now" service). See `fetch_fred.py`.
- **Download date:** 2026-07-18.
- **Units:** Billions of U.S. dollars, seasonally adjusted annual rate at source.
- **Frequency:** Quarterly.
- **Span (this file):** 1947.Q1 – 2026.Q1 (317 observations) — full history.
- **Format:** X-13 free format, one value per line. Each value is one quarter,
  in order; the first value is Q1 (January), then Q2, Q3, Q4, ...
- **Note:** treated as a raw input series for parity exercise purposes.

Spec usage (quarterly — `period = 4`):

```
series{
  title  = "US Exports of Goods and Services (EXPGS)"
  file   = "../data/expgs.dat"
  start  = 1947.1
  period = 4
}
```
