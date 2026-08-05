# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: ssmdl.f:259-278 -- the AUTOMATICALLY identified half of the same
# hold-back its sibling extra/airline_slidingspans-outlier-heldback gates for
# user-specified outliers. Different arm of the same group walk, and the arm
# does two things the other does not:
#
#   * with Ssotl==1 (`slidingspans{outlier=remove}`, the default) it RE-TYPES
#     each column -- PRGTAA->PRGTAO, PRGTAL->PRGTLS, PRGTAT->PRGTTC -- before
#     handing it to rmotss, so a span sees an ordinary outlier;
#   * it sets `regchg` on every column it touches, re-type or not, which is what
#     forces ssmdl.f:358-373's design re-snapshot.
#
# critical = 2.5 is deliberate. At the default the run finds ONE outlier
# (AO1951.May) and it happens to land where every verdict is observationally
# inert; at 2.5 the nine finds split across all three of rmotss's outcomes:
#   AO1950.Jan / TC1950.Oct / AO1950.Nov  before span 1 -> deleted outright
#   AO1951.May / TC1952.Mar / AO1952.Jun / TC1953.Apr   before the intersection
#                                                       -> held back, re-added
#                                                          only in span 1(-2)
#   AO1954.Feb                            inside the intersection -> untouched
#   AO1960.Mar                            past it -> held back, re-added in
#                                                    span 4 only
# TC is in `types` for the same reason: PRGTAT is a distinct re-type and rmotss
# gives TC the AO inequalities, not the LS ones.
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
  variables = (td)
}
outlier{
  types = (ao ls tc)
  critical = 2.5
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
