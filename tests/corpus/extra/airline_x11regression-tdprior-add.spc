# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: editor.f:1518-1530 -- prior trading-day weights are REJECTED at parse
# for an additive (or pseudo-additive) adjustment, not merely unported. The
# engine used to accept the spec and only fatal later inside x11pt1 with a
# "not yet ported" message; the oracle refuses it up front. This spec gates the
# rejection itself (test_m1_parse: OUTCOME must match the oracle's).
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  print = all
}
x11{
  mode = add
  print = all
  save = (d10 d11 d12 d13)
}
x11regression{
  tdprior = (1.4 1.4 1.4 1.4 1.4 0.5 0.5)
  print = all
}
