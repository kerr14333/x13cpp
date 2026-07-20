# Regression/outlier parity spec (reg-labor): Bell Labor-Day holiday regressor.
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
  variables = (labor[10])
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
