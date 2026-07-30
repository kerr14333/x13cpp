# Hand-authored (NOT produced by genextra.py -- do not expect a
# regeneration to recreate it).
# coverage: pickmdl{identify=first} + regression{aictest=(td)} -- the ONLY route to
#           automx.f:700-847, the post-loop restore-and-redo block, which runs
#           the AIC tests a second time behind a single ssprep (:748).
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
  save = (d10 d11 d12 d13 d16)
  savelog = all
}
