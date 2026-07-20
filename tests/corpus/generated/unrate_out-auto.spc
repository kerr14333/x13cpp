# Regression/outlier parity spec (out-auto).
series{
  title  = "US Unemployment Rate (UNRATE)"
  file   = "../data/unrate.dat"
  start  = 1948.01
  period = 12
}
transform{
  function = none
}
outlier{ }
arima{
  model = (0 1 1)
}
estimate{ }
