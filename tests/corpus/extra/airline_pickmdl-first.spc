# Hand-authored (NOT produced by genextra.py -- do not expect a
# regeneration to recreate it).
# coverage: pickmdl{method=first} -- stop at the FIRST candidate that passes every
#           screen (Pck1st), rather than the lowest-error one. Oracle
#           on-vs-off: 156 .udg keys; the model changes (0 1 2)(0 1 1) ->
#           (0 1 1)(0 1 1).
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
  mode = fcst
  file = "pickmdl.mdl"
  method = first
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
