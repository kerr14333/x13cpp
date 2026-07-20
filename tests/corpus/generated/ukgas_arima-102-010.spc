series{
  title  = "UK quarterly gas consumption (NSA)"
  file   = "../data/ukgas.dat"
  start  = 1960.1
  period = 4
}
transform{
  function = log
}
arima{
  model = (1 0 2)(0 1 0)
}
estimate{ }
