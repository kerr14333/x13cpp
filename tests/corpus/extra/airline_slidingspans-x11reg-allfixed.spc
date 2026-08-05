# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: ssxmdl.f:78-136 with Irgxfx==3 -- every x11regression coefficient
# fixed by b=(... f), so the `Irgxfx.ge.2` arm is entered, the Irgxfx==2 group
# walk is SKIPPED, and tdfx stays true -> Itd demoted to -1.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  savelog = all
}
x11{
  savelog = all
  save = (d10 d11 d12 d13 d16)
}
x11regression{
  variables = (td)
  b = (0.39f 0.39f 0.39f 0.39f 0.39f 0.25f)
  save = (xrm b16 c16)
}
slidingspans{
  print = all
  save = (sfs chs tds ads)
}
