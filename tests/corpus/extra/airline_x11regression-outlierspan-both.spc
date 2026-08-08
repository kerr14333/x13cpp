# coverage: x11regression{outlierspan=} with BOTH dates given, so neither
# default arm of gtxreg.f:665-674 runs. The start-only sibling
# (airline_x11regression-outlierspan) leaves the end on its default; this one
# pins the explicit-end branch, and the two together are what discriminate a
# port that honours only one half.
#
# Oracle AO count over the full span: 203. With this window: 28.
#
# Hand-authored; NOT produced by gen*.py.
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
  save = (d10 d11 d12 d13)
  savelog = all
}
x11regression{
  variables = (td)
  outlierspan = (1952.1,1957.12)
  critical = 3.0
  print = all
  save = (xrm b16 c16)
}
