# M2 gate: transformed (prior-adjusted) series (trn), log transform, PAYEMS.
series{
  title  = "All Employees, Total Nonfarm"
  file   = "../data/payems.dat"
  start  = 2000.01
  period = 12
  save = (a1)
}
transform{
  function = log
  save = (trn)
}
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
