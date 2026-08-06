# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: the fourth corner of the slidingspans x x11regression outlier grid,
# and the only one on which `x11outlier=` moves a number.
#
#            fixx11reg=yes                 fixx11reg=no
#  x11outl   -x11reg-autootl               -x11reg-autootl-fixno
#  =yes      (strike + re-identify)        (same, and the strike is the
#                                           difference between finishing and
#                                           the 80-regressor limit)
#  x11outl   -x11reg-x11outlier-no         THIS SPEC
#  =no       (hold back + adotss;          (hold back + adotss, and no
#             byte-identical to the         re-identification: 240 sfs lines
#             yes cell -- nothing is        away from the yes cell)
#             refit either way)
#
# It is also the only spec that reaches x11mdl.f:424-425's `Issap.eq.2.and.
# Ssxotl` guard with anything to prove. Without that clause the engine
# re-identified a fresh AO set per span ON TOP of the columns ssxmdl had
# already held back, and died at "Adding AO1958.Jan exceeds the number of
# regression effects allowed in the model (80)" -- the ssx11a strike alone does
# not save it, because on this arm the strike is not the branch taken.
# See docs/M5_PORT_NOTES.md entry 87.
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
x11regression{
  variables = (td)
  critical = 3.5
  save = (xrm b16 c16)
}
x11{
  print = all
  savelog = all
  save = (d10 d11 d12 d13 d16)
}
slidingspans{
  fixx11reg = no
  x11outlier = no
  print = all
  save = (sfs chs tds ads)
}
