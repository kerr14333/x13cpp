# Hand-authored -- NOT produced by genextra.py; do not regenerate this file.
# coverage: x11regression{aicdiff=} REJECTING the AIC-tested Easter.
#
# The sibling airline_x11regression-aictest spec exercises the ACCEPT arm of
# x11mdl.f:271-292 (`aictest.xe: yes`, window through FORMAT 2025 `(a,i3)`).
# This one exercises the REJECT arm, where the oracle writes the window as a
# literal string through FORMAT 1025 -- same key, one space instead of three,
# and only a text comparison sees the difference.
#
# aicdiff=5.0 clears the best Easter margin (easter[15] beats no-Easter by
# 2.37 AICC on this series), so x11aic.f:392 never switches away from the
# baseline and aicind stays 0.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
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
  savelog = all
}
x11regression{
  variables = (td)
  aictest = (easter)
  aicdiff = 5.0
  print = all
  save = (xrm b16 c16)
}
