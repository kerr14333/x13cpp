# coverage: transform{constant=} -- the positive shift editor.f:430 adds to the
# whole series, taken back out of the published D11/D12/original at x11pt3's
# output points (sac/tac carry the pre-removal copies). Also makes x11pt4's
# Cnstnt branches reachable (census_bugs CB-18 / CB-19).
# Hand-authored; NOT produced by genextra.py.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
  save = (b1)
}
transform{
  function = log
  constant = 50
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
  save = (d10 d11 d12 d13 d16 e1 e2 e3 e5 pe5 e6 pe6 e7 pe7
          e8 pe8 e11 e18 sac tac)
  savelog = all
}
