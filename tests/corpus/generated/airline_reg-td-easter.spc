# Regression/outlier parity spec (reg-td-easter).
series{
  title  = "Intl Airline Passengers"
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
