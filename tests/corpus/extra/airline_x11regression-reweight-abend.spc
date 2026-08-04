# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: x11mdl.f:613-623 -- the reweight has a negative daily weight to fix and
# NO positive unfixed weight to rescale, so there is no factor to build and the
# oracle abends. One notch above -reweight (0.43 vs 0.39 on the five fixed
# contrasts): Saturday's own estimate crosses -1, so its weight is zeroed alongside
# Sunday's and tdwsum comes out 0.
#
# The pair matters. A reweight that silently did nothing would still pass
# -reweight-off; only this spec says the abend arm exists.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
  save = (b1)
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
  save = (d10 d11 d12 d13)
  savelog = all
}
x11regression{
  variables = (td)
  b = (0.43f 0.43f 0.43f 0.43f 0.43f 0.25)
  reweight = yes
  save = (xrm)
}
