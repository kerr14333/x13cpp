# HAND-AUTHORED -- NOT produced by genspecs.py. Do not delete when regenerating.
# series = airline, config = hp-short-seats
#
# The hpcycle == -1 "auto" sentinel (the DEFAULT -- gtinpt.f:536 defaults Lhp
# TRUE, so every seats{} run reaches sigex.f:2370 with -1) resolves against the
# SERIES LENGTH at sigex.f:2371-2386: monthly needs nz >= 120. This span is 72
# observations, so hpcycle resolves to 0 and the oracle writes no cyc/ltt --
# the only way to gate the minimum-span rule. No hp* arguments at all: the spec
# is the DEFAULT configuration, short.
# Gated by tests/parity/test_seats_hpopts.py.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
  span = (1955.01, 1960.12)
  save = (a1 b1)
}
transform{
  function = log
  save = (trn)
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  save = (mdl est lks)
}
seats{
  save = (s12 s10 s13 s11 s16 s18 mdc cyc ltt)
}
