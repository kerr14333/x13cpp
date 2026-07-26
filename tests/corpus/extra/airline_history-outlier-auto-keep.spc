# coverage: rmotrv.f over the outliers an outlier{} spec identified on the MAIN
# run. They are ordinary regression columns by the time history{} runs, so the
# same hold-back applies -- and with the default `outlier=keep` that is the ONLY
# thing that happens to them (revdrv.f:275-287 switches the per-span automatic
# identification off).
# critical = 3.0 matters: at the default the airline series has no outlier over
# the threshold, and a probe run at 3.5 measured 0.000e+00 for every history
# outlier flag -- a saturated precondition, not a null. With finds present the
# engine was off by sar 9.2e-1 / sae 1.5e-2 before rmotrv landed.
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
outlier{
  types = (ao ls)
  critical = 3.0
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
