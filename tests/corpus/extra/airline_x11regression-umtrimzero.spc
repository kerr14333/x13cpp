# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: gtxreg.f's computed GO TO, argument 30. This port dispatched the
# CENTERUSER reader (label 310, URRDIC mean/seasonal) on argidx 30, which is
# UMTRIMZERO (label 300, ZRODIC yes/span/no) -- an off-by-one that ran both ways:
# `umtrimzero = seasonal` was accepted at OUTCOME: OK where the oracle errors, and a
# real `centeruser=` fell through to the generic consume and was discarded.
#
# `seasonal` is the discriminator: legal for centeruser, illegal for umtrimzero. The
# oracle refuses the run; before the fix the engine adjusted the series.
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
estimate{}
x11{}
x11regression{
  variables = (td)
  umtrimzero = seasonal
}
