# Explicit-model AIC regressor test (aictest=(td)) -- arima.f:569 path.
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
  aictest = (td)
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
