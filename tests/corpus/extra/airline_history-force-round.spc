# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: x11pt3.f:903-918 -- with force{round=yes} the span store takes the
# ROUNDED series Stcirn, and BOTH sites there were missing: the sliding-spans
# ssrit at :903-906 and the revisions getrev at :915-918. The block was ported
# with "(deferred: rnd table/punch; ssrit/getrev stores.)" -- a comment, not a
# wall, so it appeared in no inventory. This is the third and last of the three
# SA store sites (Iyrt==0 at :677, forced at :820, rounded here).
#
# Note the getrev call sits OUTSIDE the `rndok` branch in the Fortran, so a
# rounding that failed still stores; transcribed that way.
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
  save = (d10 d11 d12 d13 d16)
  savelog = all
}
force{
  type = regress
  target = original
  round = yes
  rho = 0.9
  print = all
  save = (saa rnd ffc)
}
history{
  estimates = (sadj sadjchng trend trendchng)
  start = 1956.jan
  print = all
  save = (sar sae chr che trr tre tcr tce)
  savelog = all
}
