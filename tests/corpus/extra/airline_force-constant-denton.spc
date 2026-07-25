# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: same negative-value correction as airline_force-constant-regress, but
# reached from the Denton branch (Iyrt==1) -- x11pt3.f's correction runs AFTER the
# qmap/qmap2 if-else, so the fallback pass is qmap2 either way.
series{
  title = "Airline, shifted to cross zero"
  file = "../data/airline_zero.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
  constant = 100
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  print = all
  savelog = all
}
x11{
  print = all
  save = (d10 d11 d12 d13 d16)
  savelog = all
}
force{
  type = denton
  print = all
  save = (saa ffc)
}
