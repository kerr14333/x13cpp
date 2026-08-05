# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: ssmdl.f:71-119's `Iregfx.eq.2` group walk, NEGATIVE side. One TD
# column is left FREE, so Tdfix ANDs down to false and Itd is NOT demoted.
# The A/B partner of airline_slidingspans-regfixed: same spec, one `f` removed,
# and the whole observable difference is that .tds/.ads exist here.
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
  b = (0.001f 0.001f 0.001f 0.001f 0.001f 0.001)
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
slidingspans{
  fixmdl = no
  print = all
  save = (sfs chs tds ads)
}
