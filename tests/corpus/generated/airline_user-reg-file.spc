# User-defined regressor read from an external free-format file (file=).
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
  user = (u1 u2)
  file = "../data/userreg2.dat"
}
arima{
  model = (0 1 1)
}
estimate{ }
