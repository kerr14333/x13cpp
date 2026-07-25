# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: x11pt3.f:750-782 force{} negative-value correction (the second qmap2
# pass) + x11pt3.f:841-850's negfin forcing-factor branch. Reachable only with
# transform{constant=} in a non-additive mode, so the series is airline shifted
# down by 150 (24 observations at or below zero) and lifted back by constant=100.
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
  type = regress
  print = all
  save = (saa ffc)
}
