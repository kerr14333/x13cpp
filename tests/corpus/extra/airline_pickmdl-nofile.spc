# Hand-authored (NOT produced by genextra.py -- do not expect a
# regeneration to recreate it).
# coverage: pickmdl{} with NO file= -- the five BUILT-IN X-11-ARIMA
#           candidates (setamx.f), and the CNOTST-in-first-character
#           `havfil` test that selects them. `mdlfile: no` in the golden.
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
