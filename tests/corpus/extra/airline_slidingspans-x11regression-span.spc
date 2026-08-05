# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: slidingspans{} + x11regression{span=}, i.e. ssxmdl.f:27-39 -- the arm
# that FORCES Ssxint on regardless of what fixx11reg= said, prints a three-line
# NOTE, and demotes Itd/Ihol from 1 to -1 ("requested but not done").
#
# Landed because the arm was PORTED but UNGATED: nothing in the corpus made
# Begxrg later than Begspn, so its NOTE text and its Itd/Ihol demote were both
# unverified.
#
# KEEP tds AND ads IN THE SAVE LIST even though the oracle writes neither. The
# demote's whole observable is that ABSENCE, and it is only a gate because the
# save list names them: test_slidingspans_table then asserts the engine
# produces zero cells for a tag the spec asked for and the oracle declined.
# Dropping them from the save list silently deletes that assertion.
#
# What gates besides: sfs / chs (600 cells each, ~5e-15), b16/c16/xrm, the
# D-tables, and -- via test_slidingspans_notes -- the two NOTE blocks on the
# .err channel verbatim, ssxmdl.f:35-39's and ssphdr.f:145-152's.
#
# The companion spec airline_slidingspans-x11regression.spc has no span= and
# therefore reaches the same tail (Ssxint from the fixx11reg= default) by the
# other route; the pair separates "the default fixed the coefficients" from
# "the span= forced them fixed and cancelled the TD analysis". It is also the
# NOTE-free half of that gate -- see docs/M5_PORT_NOTES.md entry 81.
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
  savelog = all
}
x11{
  savelog = all
  save = (d10 d11 d12 d13 d16)
}
x11regression{
  variables = (td)
  span = (1951.1, )
  save = (xrm b16 c16)
}
slidingspans{
  print = all
  save = (sfs chs tds ads)
}
