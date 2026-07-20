# Fixed-model parity spec (model-order coverage for the M3 gate).
series{
  title  = "US Unemployment Rate (UNRATE, SA)"
  file   = "../data/unrate.dat"
  start  = 1948.01
  period = 12
}
transform{
  function = log
}
arima{
  model = (2 1 0)
}
estimate{ }
