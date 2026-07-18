# M2 gate: leap-year prior adjustment (a2 factors, a3 prior-adjusted, trn).
series{
  title  = "All Employees, Total Nonfarm"
  file   = "../data/payems.dat"
  start  = 2000.01
  period = 12
  save = (a1)
}
transform{
  function = log
  adjust   = lpyear
  save = (a2 a3 trn)
}
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
x11{ }
