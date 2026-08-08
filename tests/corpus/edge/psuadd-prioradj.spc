# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: editor.f:2523-2527, arm 3 of 4 -- prior adjustment factors cannot be
# combined with pseudo-additive adjustment. The trigger is Priadj>1, and
# `transform{adjust=lom}` sets Priadj=2 on a monthly series (getadj.f's ADJDIC
# maps none/lom/loq/lpyear to 1..4, then normalises lom/loq to the period).
#
# Reached with no regression{} and no x11regression{}, so arms 1 and 2 are both
# false and this spec is provably on arm 3 -- the three arms differ only by
# which flag they read and a port that wired one of them passes a spec written
# for another.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  adjust = lom
}
x11{
  mode = pseudoadd
  save = (d10 d11 d12 d13)
}
