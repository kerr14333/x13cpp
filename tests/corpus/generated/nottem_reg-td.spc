# Regression/outlier parity spec (reg-td).
series{
  title  = "Nottingham temperature (NSA)"
  file   = "../data/nottem.dat"
  start  = 1920.01
  period = 12
}
transform{
  function = none
}
regression{
  variables = (td)
}
arima{
  model = (1 0 0)(1 1 1)
}
estimate{ }
