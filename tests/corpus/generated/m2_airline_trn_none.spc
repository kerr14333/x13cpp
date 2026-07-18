# M2 gate: transformed (prior-adjusted) series (trn), identity (none) transform.
series{
  title  = "International Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
  save = (a1)
}
transform{
  function = none
  save = (trn)
}
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
