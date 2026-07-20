# Regression/outlier parity spec (out-auto).
series{
  title  = "US Nonfarm Employment (PAYEMS)"
  file   = "../data/payems.dat"
  start  = 2000.01
  period = 12
}
transform{
  function = log
}
outlier{ }
arima{
  model = (0 1 2)
}
estimate{ }
