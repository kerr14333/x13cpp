# Hand-authored (NOT produced by genextra.py -- do not expect a
# regeneration to recreate it).
# coverage: pickmdl{fcstlim=0} -- nothing can be accepted, so the STARRED
#           default model in pickmdl.mdl is used anyway to produce the
#           regARIMA preadjustment factors (hvstar==2), with forecasting
#           switched off (nofcst.f).
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
  fcstlim = 0
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
