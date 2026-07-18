# M2 gate: length-of-month prior adjustment (a2 factors, a3 prior-adjusted, trn).
series{
  title  = "International Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
  save = (a1)
}
transform{
  function = log
  adjust   = lom
  save = (a2 a3 trn)
}
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
x11{ }
