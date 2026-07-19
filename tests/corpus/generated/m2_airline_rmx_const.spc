# M2 gate: regression design matrix (rmx) -- Constant column integrated
# against the airline differencing (1-B)(1-B^12) via ratpos.
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
  variables = (const)
  save = (rmx)
}
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
