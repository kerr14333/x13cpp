# M2 gate: regression design matrix (rmx) -- td WITHOUT a log transform keeps
# the implicit additive Leap Year column (td7var) instead of the prior.
series{
  title  = "International Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
  save = (a1)
}
regression{
  variables = (const td)
  save = (rmx)
}
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
