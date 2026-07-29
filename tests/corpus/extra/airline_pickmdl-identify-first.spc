# Hand-authored (NOT produced by genextra.py -- do not expect a
# regeneration to recreate it).
# coverage: pickmdl{identify=first} + outlier{} -- outlier identification on the
#           FIRST candidate only, which makes automx.f's label-20 re-
#           estimation reachable (the winner was fitted against candidate
#           1's outlier columns and has to be refitted against the
#           original design). Oracle on-vs-off: 189 keys.
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
  method = best
  identify = first
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
outlier{
  critical = 3.0
}
