# M2 gate: regression design matrix (rmx) -- Constant + seasonal contrast
# columns (addsef) with a nonseasonal-only (0 1 1) model.
series{
  title  = "International Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
  save = (a1)
}
transform{
  function = log
  save = (trn)
}
regression{
  variables = (const seasonal)
  save = (rmx)
}
arima{ model = (0 1 1) }
estimate{ }
