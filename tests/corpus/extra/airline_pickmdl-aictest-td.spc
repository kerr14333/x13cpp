# Hand-authored (NOT produced by genextra.py -- do not expect a
# regeneration to recreate it).
# coverage: pickmdl{} + regression{aictest=(td)} -- the AIC-regressor tests run
#           INSIDE the candidate loop (automx.f:404-500), replacing the plain
#           rgarma for every identified candidate. The oracle picks
#           1-Coefficient Trading Day for all five candidates; nreg 1, and
#           aictest.diff.td 20.197 survives from the LAST candidate, not the
#           winner.
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
