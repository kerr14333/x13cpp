# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: the A/B control for -reweight. Identical except `reweight = no`, which
# is also the default -- so this is what the engine produced for BOTH specs while
# reweight= was parsed and discarded. 80 c16 lines and 290 d11 lines apart.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
  save = (b1)
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  savelog = all
}
x11{
  save = (d10 d11 d12 d13 d16)
  savelog = all
}
x11regression{
  variables = (td)
  b = (0.39f 0.39f 0.39f 0.39f 0.39f 0.25)
  reweight = no
  save = (xrm b16 c16)
}
