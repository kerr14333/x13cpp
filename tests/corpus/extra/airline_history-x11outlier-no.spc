# coverage: history{} + x11regression{critical=} with x11outlier = NO -- the
# other arm of revdrv.f:332-350. With yes (the default, gated by
# airline_history-x11outlier) rmatot DELETES the automatically identified
# x11regression outliers so every span re-identifies its own; with no, rmotrv
# holds them back by date instead and x11mdl.f:424's outlier-ID arm is switched
# off for the whole span loop.
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
