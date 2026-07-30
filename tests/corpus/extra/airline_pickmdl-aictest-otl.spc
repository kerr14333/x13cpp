# Hand-authored (NOT produced by genextra.py -- do not expect a
# regeneration to recreate it).
# coverage: pickmdl{identify=all} + regression{aictest=(td)} + outlier{} -- the
#           AIC tests inside the candidate loop AND the per-candidate design
#           restore at automx.f:302-324. The outlier{} spec is what makes the
#           restore reachable at all: `lidotl` is `Ltstao.or.Ltstls.or.Ltsttc`
#           (arima.f:118), so without one, pickmdl{identify=} is inert and the
#           restore branch never runs -- measured, by a mutation that deleted
#           the restore and passed the whole suite.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
}
regression{
  aictest = (td)
  print = all
}
pickmdl{
  mode = fcst
  file = "pickmdl.mdl"
  method = best
  identify = all
  print = all
}
outlier{
  critical = 3.0
  print = all
}
estimate{
  print = all
  savelog = all
}
forecast{
  maxlead = 12
  print = all
}
x11{
  print = all
  save = (d10 d11 d12 d13 d16)
  savelog = all
}
