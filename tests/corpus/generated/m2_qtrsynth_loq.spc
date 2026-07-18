# M2 gate: length-of-quarter prior adjustment on a synthetic quarterly series.
series{
  title  = "Synthetic quarterly series"
  file   = "qtrsynth.dat"
  start  = 2000.1
  period = 4
  save = (a1)
}
transform{
  function = log
  adjust   = loq
  save = (a2 a3 trn)
}
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
x11{ }
