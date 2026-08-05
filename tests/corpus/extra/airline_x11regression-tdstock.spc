# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: the NON-firing side of x11mdl.f:661-690's stock trading-day abend,
# and the first x11regression STOCK trading-day design in the corpus at all.
# Same shape as airline_x11regression-tdstock-abend with coefficients that keep
# the daily factors positive: this one must reach OUTCOME: OK and gate its
# b16/c16/xrm and D-tables.
#
# A refusal gated only on the side that fires cannot distinguish "correctly
# refused" from "refuses everything" -- the saturated-precondition trap. This is
# the other side.
series{
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
arima{
  model = (0 1 1)(0 1 1)
}
x11{
  save = (d10 d11 d12 d13)
}
x11regression{
  variables = (tdstock[15])
  save = (xrm b16 c16)
}
