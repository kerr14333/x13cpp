# coverage: the NEGATIVE control for rmotrv.f -- outlier regressors dated BEFORE
# the first revision date are not held back, and this spec was already correct
# before the port (measured 3.4e-4 / 3.4e-6, the per-span floor, both ways). It
# is what pins the hold-back to `begotl > Begrev` rather than to "an outlier
# exists".
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
regression{
  variables = (ao1950.may ls1953.jun)
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
history{
  estimates = (sadj sadjchng trend trendchng)
  start = 1955.jan
  print = all
  save = (sar sae chr che trr tre tcr tce)
  savelog = all
}
