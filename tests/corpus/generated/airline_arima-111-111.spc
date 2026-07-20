# Fixed-model parity spec (model-order coverage for the M3 gate).
series{
  title  = "International Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{
  function = log
}
arima{
  model = (1 1 1)(1 1 1)
}
estimate{ }
