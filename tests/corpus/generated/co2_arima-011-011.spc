series{
  title  = "Mauna Loa CO2 (NSA)"
  file   = "../data/co2.dat"
  start  = 1959.01
  period = 12
}
transform{
  function = none
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
