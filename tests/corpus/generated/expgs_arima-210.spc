# Fixed-model parity spec (model-order coverage for the M3 gate).
series{
  title  = "US Exports of Goods and Services"
  file   = "../data/expgs.dat"
  start  = 1947.1
  period = 4
}
transform{
  function = log
}
arima{
  model = (2 1 0)
}
estimate{ }
