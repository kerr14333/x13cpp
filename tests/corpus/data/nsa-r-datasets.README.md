# NSA seasonal datasets from R's `datasets` package

Genuinely **non-seasonally-adjusted**, strongly seasonal series, extracted from
the base R `datasets` package (public-domain classics) so the seasonal ARIMA /
seasonal-adjustment / automdl code paths can be tested on data that actually has
seasonality — unlike the FRED PAYEMS/UNRATE/EXPGS series, which are SA at source.
No network needed; regenerate with the R snippet at the bottom.

| file           | R dataset     | description                              | n   | period | start   | transform |
|----------------|---------------|------------------------------------------|-----|--------|---------|-----------|
| `nottem.dat`   | `nottem`      | Nottingham avg monthly air temperature   | 240 | 12     | 1920.01 | none      |
| `ukgas.dat`    | `UKgas`       | UK quarterly gas consumption             | 108 | 4      | 1960.1  | log       |
| `usdeaths.dat` | `USAccDeaths` | US monthly accidental deaths             | 72  | 12     | 1973.01 | none      |
| `co2.dat`      | `co2`         | Mauna Loa monthly atmospheric CO2 (ppm)  | 468 | 12     | 1959.01 | none      |

Values are X-13 free format (one value per line, 4 decimal places, date order,
no header). Oracle-confirmed automdl identifications (seasonal differencing D=1
now exercised): nottem `(1 0 0)(1 1 1)`, ukgas `(1 0 2)(0 1 0)`,
co2 `(0 1 1)(0 1 1)`, usdeaths `(0 1 1)(0 1 1)`.

Regenerate (needs R; no network):

```r
out <- "tests/corpus/data"
for (m in list(c("nottem","nottem.dat"), c("UKgas","ukgas.dat"),
               c("USAccDeaths","usdeaths.dat"), c("co2","co2.dat"))) {
  v <- as.numeric(get(m[1]))
  writeLines(formatC(v, format="f", digits=4), file.path(out, m[2]))
}
```
