# coverage: x11pt1.f:229-230's ENTRY CONDITION for the prior trading-day block.
#
#   Kswv==1 .and. (((Axrghl.or.Axrgtd).and.Ixreg==3) .or. Khol<2)
#
# With the classic X-11 Easter on, gtinpt.f:1239's `Khol=Keastr` puts Khol at 2,
# and with no x11-regression prior calendar the first disjunct is false -- so the
# oracle SKIPS the whole prior-TD block and adjusts with no prior TD at all. It
# does NOT reject the spec: it writes d10/d11 normally.
#
# The engine used to FATAL here ("x11pt1 prior trading-day adjustment
# (pritd/ssrit) not yet ported"), because the guard tested only `Khol>=2` and
# could not tell "unported branch" from "branch the oracle declines to enter".
# Measured before the fix: oracle wrote 144 d10 rows, engine wrote none.
#
# What still fatals, correctly: entering the block WITH Khol>=2 via the
# x11-regression arm, which needs the classic-Easter user-weight combine.
#
# Hand-authored; NOT produced by gen*.py.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
x11{
  print = all
  save = (d10 d11 d12 d13)
  savelog = all
  x11easter = yes
}
x11regression{
  tdprior = (1.0 1.0 1.0 1.0 1.0 1.0 1.0)
  print = all
}
