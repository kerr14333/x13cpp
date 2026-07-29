# Hand-authored (NOT produced by genextra.py -- do not expect a
# regeneration to recreate it).
# coverage: pickmdl{overdiff=} -- the MA-sum overdifferencing screen, the one
#           screen of the three that reads the estimated COEFFICIENTS rather
#           than a forecast statistic. Oracle on-vs-off: 168 .udg keys.
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
  overdiff = 0.1
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
