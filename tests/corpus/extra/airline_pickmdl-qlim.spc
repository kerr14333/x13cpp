# Hand-authored (NOT produced by genextra.py -- do not expect a
# regeneration to recreate it).
# coverage: pickmdl{qlim=} -- the Ljung-Box p-value floor. At 90 percent almost
#           every candidate is rejected, so this pins the SCREEN rather than
#           the ranking. Oracle on-vs-off: 160 .udg keys.
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
  variables = (td)
  print = all
}
pickmdl{
  qlim = 90
  mode = fcst
  file = "pickmdl.mdl"
  method = best
  identify = all
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
  savelog = all
}
