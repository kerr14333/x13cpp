# coverage: revchk.f:1053-1110's validation of the alternate revision targets.
# `sadjlags = (6 3 72)` is deliberately UNSORTED and carries one lag that does
# not fit: the revision span is 71 periods (1955.01..1960.12), so intsrt sorts
# the list to (3 6 72), the 72 is dropped with a NOTE, and the SA tables come
# back with two target columns in ascending order. Neither list is a 1yr/2yr
# pair, so Lr1y2y is false and there is no extra (1yr-2yr) column anywhere.
#
# The dropped lag still matters: revchk calls setrvp BEFORE it validates, so
# Endsa is widened by mxrlag=72 (capped at the last observation) even though 72
# is about to be discarded -- and Endsa is what prtrev's DNOTST mask keys on.
# `trendlags = (6)` covers the single-lag trend family.
# Hand-authored; NOT produced by gen*.py.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
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
}
history{
  estimates = (sadj sadjchng trend trendchng)
  start = 1955.jan
  sadjlags = (6 3 72)
  trendlags = (6)
  print = all
  save = (sar sae chr che trr tre tcr tce)
  savelog = all
}
