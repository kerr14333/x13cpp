# coverage: history{estimates=(aic arma td)} -- the three MODEL histories
# (lkh = log likelihood + AICC, amh = the free ARMA coefficients, tdh = the free
# trading-day coefficients plus each TD group's implied Sunday contrast).
# Hand-authored; NOT produced by genextra.py.
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
  variables = (td)
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
  estimates = (aic arma td)
  start = 1955.jan
  print = all
  save = (lkh amh tdh)
  savelog = all
}
