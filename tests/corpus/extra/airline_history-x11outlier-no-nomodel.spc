# coverage: the MODEL-FREE twin of airline_history-x11outlier-no -- no
# transform{}, arima{} or estimate{}, so nothing re-estimates a regARIMA model
# per span and the only thing moving is the x11regression outlier bookkeeping.
# Its x11outlier=yes sibling (airline_history-x11outlier-nomodel) gates
# BIT-EXACT, which is what makes this one the clean discriminator for the
# x11outlier=no arm.
# Hand-authored; NOT produced by gen*.py.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
x11{
  print = all
  savelog = all
}
x11regression{
  variables = (td)
  critical = 3.0
  print = all
}
history{
  estimates = (sadj trend)
  start = 1955.jan
  x11outlier = no
  print = all
  save = (sar sae trr tre)
  savelog = all
}
