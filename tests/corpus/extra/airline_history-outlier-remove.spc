# coverage: history{outlier=remove} (Otlrev=1) -- rmatot.f. Every outlier the
# MAIN run's outlier{} identified is struck from the design outright, so no span
# inherits a find it could not have made itself. (`keep`, the default, holds
# them back by date instead; see airline_history-outlier-auto-keep.)
# OTLDIC is 'keepremoveauto', so `keep` is the default even though gtrvst's own
# error message lists remove first.
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
  outlier = remove
  print = all
  save = (sar sae chr che trr tre tcr tce)
  savelog = all
}
