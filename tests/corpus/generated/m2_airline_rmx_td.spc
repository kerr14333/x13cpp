# M2 gate: regression design matrix (rmx) -- Constant + six trading-day
# contrasts (td6var); log transform triggers the implicit leap-year prior
# (rmlnvr -> Priadj=4), so a2/a3/trn cover the prior chain too.
series{
  title  = "International Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
  save = (a1)
}
transform{
  function = log
  save = (a2 a3 trn)
}
regression{
  variables = (const td)
  save = (rmx)
}
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
