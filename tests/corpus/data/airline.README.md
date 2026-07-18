# airline.dat — Box-Jenkins Series G

- **Series:** International airline passengers, monthly totals.
- **Source:** Box, G.E.P. and Jenkins, G.M. (1976), *Time Series Analysis:
  Forecasting and Control*, Series G. Classic public-domain dataset (also
  distributed as `AirPassengers` in R).
- **Vintage:** Published values, invariant. Hardcoded in this repo (not
  downloaded).
- **Units:** Number of passengers, in thousands.
- **Frequency:** Monthly.
- **Span:** 1949.01 – 1960.12 (144 observations).
- **Format:** X-13 free format, one value per line.

Spec usage:

```
series{
  title  = "International Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
```

This is the canonical airline-model series (log transform, `(0 1 1)(0 1 1)`
ARIMA), used throughout the X-13 documentation as the "Getting Started" example.
