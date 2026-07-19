# M2 gate: regression design matrix (rmx) -- Constant + td + Easter[8]
# (estrmu long-term means + adestr proportions).
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
  variables = (const td easter[8])
  save = (rmx)
}
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
