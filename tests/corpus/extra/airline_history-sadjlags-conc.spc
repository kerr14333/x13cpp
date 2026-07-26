# coverage: history{target=concurrent} (Cnctar) on top of the alternate revision
# targets. prtrev.f:181-186 flips what every target column measures: the default
# is Fin(0)-Fin(lag), the revision from the "lag later" estimate TO the final
# one; with target=concurrent it is Fin(lag)-Conc, the revision FROM the
# concurrent estimate. Column 0 (final-vs-concurrent) is unchanged either way,
# and the DNOTST mask moves one row later (prtrev.f:178).
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
  savelog = all
}
history{
  estimates = (sadj sadjchng trend trendchng)
  start = 1955.jan
  sadjlags = (12 24)
  trendlags = (12 24)
  target = concurrent
  print = all
  save = (sar sae chr che trr tre tcr tce)
  savelog = all
}
