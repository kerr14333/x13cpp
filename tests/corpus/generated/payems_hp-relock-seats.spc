# HAND-AUTHORED -- NOT produced by genspecs.py. Do not delete when regenerating.
# series = payems, config = hp-relock-seats
#
# Hodrick-Prescott option bridge, BOTH interlocks at once:
#  (1) CENSUS BUG CB-14 (ansub9.f:1112-1116) -- the `hplan=` block re-enables
#      the HP filter without asking WHY hpcycle was 0, so the explicit
#      `hpcycle=no` below is silently overridden and the filter runs anyway.
#  (2) with `hptarget=` set, that re-enable targets Hptrgt (3 = split the
#      ORIGINAL series, sigex.f:2466) rather than the trend (1).
# The oracle echoes the resolved `hpcycle= 3` in its .sum INPUT block, which is
# what tests/parity/test_seats_hpopts.py gates against.
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
  hpcycle = no
  hplan = 40
  hptarget = orig
  save = (s12 s10 s13 s11 s16 s18 mdc cyc ltt)
}
