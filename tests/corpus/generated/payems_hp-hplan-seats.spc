# HAND-AUTHORED -- NOT produced by genspecs.py. Do not delete when regenerating.
# series = payems, config = hp-hplan-seats
#
# Hodrick-Prescott option bridge (ansub9.f:1109-1117): `hplan=` alone sets
# L_hplan AND forces L_hpcycle from the -1 "auto" sentinel to 1. No `print=`
# in seats{} on purpose -- ansub9.f:1050 would push L_OUT to 3 and suppress
# HPOUTPUT's cyc/ltt tables entirely. Gated by
# tests/parity/test_seats_hpopts.py (option resolution) and, for the ordinary
# decomposition tables, by tests/parity/test_seats_tables.py: hplan provably
# does NOT move s10-s18 (its whole blast radius is .cyc/.ltt/.tbs/.sum).
series{
  title = "US Total Nonfarm Employment (PAYEMS)"
  file = "../data/payems.dat"
  start = 2000.01
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
  hplan = 40
  save = (s12 s10 s13 s11 s16 s18 mdc cyc ltt)
}
