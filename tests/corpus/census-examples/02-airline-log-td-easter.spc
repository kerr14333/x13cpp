# Census X-13ARIMA-SEATS manual — canonical airline example.
# Log transform, trading-day + Easter regression, (0 1 1)(0 1 1) airline
# ARIMA model, then X-11 seasonal adjustment using the regARIMA extension.
series{
  title  = "International Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{
  function = log
}
regression{
  variables = (td easter[8])
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
forecast{
  maxlead = 12
}
x11{ }
