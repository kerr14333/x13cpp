series{
  title  = "US monthly accidental deaths (NSA)"
  file   = "../data/usdeaths.dat"
  start  = 1973.01
  period = 12
}
transform{
  function = none
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
