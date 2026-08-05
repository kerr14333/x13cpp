# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: ssmdl.f:246-253's rmotss (hold back the outlier regressors a sliding
# span cannot estimate) + ssx11a.f:220-270's per-span delete/adotss + the
# sspdrv.f:208-219 strip that follows each span.
#
# THE TWO DATES ARE BOTH LOAD-BEARING, and they take DIFFERENT branches of
# rmotss. With airline (1949.Jan-1960.Dec) the four spans are 1951.Jan-1957.Dec,
# 1952.Jan-1958.Dec, 1953.Jan-1959.Dec and 1954.Jan-1960.Dec, so
#
#   ao1950.feb  is before the FIRST span starts -> rmotss.f:36-40, deleted
#               outright and never stored. Observationally inert on its own,
#               because Ssinit==1 fixes the coefficient and the column is all
#               zeros inside every span window anyway -- it is here to keep that
#               branch executed, not because it moves a number.
#   ao1959.nov  is past the span INTERSECTION (which ends 1957.Dec) -> stored,
#               deleted, and re-added by adotss only in the spans that reach it
#               (3 and 4). This one is what the gate is for: before the port the
#               engine kept it in every span and read 3.1e-03 / 3.3e-03 out in
#               sfs on spans 3-4 and 4.2e+0 / 6.0e+0 in chs, at OUTCOME: OK.
#
# Deleting the two ao= terms reproduces neither divergence (bit-exact), so the
# owner is the hold-back and not the presence of a regression{} group.
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
  variables = (ao1950.feb ao1959.nov td)
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
  savelog = all
  save = (d10 d11 d12 d13 d16)
}
slidingspans{
  print = all
  save = (sfs chs)
}
