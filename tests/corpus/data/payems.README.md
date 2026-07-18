# payems.dat — US total nonfarm employment (FRED PAYEMS)

- **Series:** All Employees, Total Nonfarm (`PAYEMS`).
- **Source:** U.S. Bureau of Labor Statistics via FRED, St. Louis Fed.
  Canonical URL: `https://fred.stlouisfed.org/graph/fredgraph.csv?id=PAYEMS`
- **Access:** Retrieved from the Internet Archive Wayback Machine capture
  `20250909155823` (byte-identical to the FRED CSV; direct FRED access was
  blocked from the build environment). See `fetch_fred.py`.
- **Download date:** 2026-07-18.
- **Units:** Thousands of persons.
- **Frequency:** Monthly.
- **Span (this file):** 2000.01 – 2025.08 (308 observations). The FRED series
  begins 1939.01; per the corpus spec it is trimmed to 2000.01 onward.
- **Note:** The FRED headline PAYEMS is seasonally adjusted at source; here it
  is treated simply as an input series for exercising the code paths (this is a
  test corpus for parity, not an economic analysis).
- **Format:** X-13 free format, one value per line.

Spec usage:

```
series{
  title  = "US Total Nonfarm Employment (PAYEMS)"
  file   = "../data/payems.dat"
  start  = 2000.01
  period = 12
}
```
