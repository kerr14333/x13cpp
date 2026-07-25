# HAND-AUTHORED -- NOT produced by genspecs.py. Do not delete when regenerating.
# series = airline, config = hp-off-seats
#
# Hodrick-Prescott option bridge (ansub9.f:1080-1090): `hpcycle=no` clears Lhp
# (defaulted TRUE at gtinpt.f:536), so L_hpcycle resolves to 0 and the oracle
# writes no cyc/ltt at all -- the ONE spec shape in this family where the
# oracle itself produces nothing. No `print=` in seats{} on purpose:
# ansub9.f:1050 would push L_OUT to 3 and suppress HPOUTPUT regardless, which
# would make the "off" case indistinguishable from the "on" case.
# Gated by tests/parity/test_seats_hpopts.py.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
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
  hpcycle = no
  save = (s12 s10 s13 s11 s16 s18 mdc cyc ltt)
}
