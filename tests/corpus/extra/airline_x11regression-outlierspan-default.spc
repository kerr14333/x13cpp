# coverage: the DEFAULT outlier-test window, which is not the one it looks like.
# There is no outlierspan= here on purpose. gtxreg.f:671 defaults Endxot to
# `Begsrs + Nobs - 1` -- the end of the SERIES -- while Begxot defaults to
# Begspn, the start of the span. With the `series{span=}` below the two differ
# by two years.
#
# The observable is NOT the search window: idotlr clamps its end to Nspobs
# (`iedtst`, and idotlr.f:212 clamps identically), so a window running past the
# span end tests exactly the same points. It is the CRITICAL VALUE:
# editor.f:1749-1757 derives Critxr from the outlier-span LENGTH when no
# `critical=` was given. Hence no `critical=` here -- and hence `easter[8]`,
# because with a trading-day group and nothing else editor.f:1730 takes the
# Sigxrg=2.5 arm and never switches Otlxrg on at all.
#
# The SPAN is chosen, not arbitrary. Most spans put the two derived critical
# values on either side of no outlier at all: 1950.1-1958.12 gives 3.87 against
# 3.83 and the adjustment is bit-identical, which is how a first draft of this
# spec came to measure a zero for a mutation that really does change Critxr.
# 1951.1-1954.12 (120 months to the series end against the span's 48) moves d11
# by 1.8e-2 relative. A window difference that changes no verdict gates nothing.
#
# Hand-authored; NOT produced by gen*.py.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  span = (1951.1,1954.12)
  period = 12
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  print = all
  savelog = all
}
x11{
  print = all
  save = (d10 d11 d12 d13)
  savelog = all
}
x11regression{
  variables = (td easter[8])
  print = all
  save = (xrm b16 c16)
}
