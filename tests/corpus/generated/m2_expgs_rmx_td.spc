# M2 gate: regression design matrix (rmx) -- quarterly Constant + td
# (td6var quarterly tables; implicit leap-year prior via log).
series{
  title  = "US Exports of Goods and Services (EXPGS)"
  file   = "../data/expgs.dat"
  start  = 1947.1
  period = 4
  save = (a1)
}
transform{
  function = log
  save = (trn)
}
regression{
  variables = (const td)
  save = (rmx)
}
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
