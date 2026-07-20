# Fixed-model parity spec (model-order coverage for the M3 gate).
series{
  title  = "US Total Nonfarm Employment (PAYEMS, SA)"
  file   = "../data/payems.dat"
  start  = 2000.01
  period = 12
}
transform{
  function = log
}
arima{
  model = (0 1 2)
}
estimate{ }
