# User-specified outlier regressor parity spec.
series{
  title = "Airline"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{ function = log }
regression{
  variables = (rp1954.jan-1956.dec)
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
