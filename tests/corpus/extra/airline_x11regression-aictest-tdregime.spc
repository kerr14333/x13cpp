# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: editor.f:1811-1822, the ONLY writer of Xaicrg -- the change-of-
# regime date for the trading-day AIC test, recovered by string search over the
# x11regression group TITLES rather than carried from the spec.
#
# The date has to come back through the title TEXT, and editor.f tries four
# shapes in order -- '(before ', '(change for before ', '(starting ',
# '(change for after ' -- each as `index(...)+k` followed by `IF(rgmgrp.eq.k)`,
# i.e. "k means not found". This spec's titles are
#   Trading Day (after 1955.Jan) + Trading Day (change for before 1955.Jan)
# so the FIRST search misses ('(before ' is not a substring of
# '(change for before ') and the SECOND hits: the spec exercises the
# fall-through, not just the head of the chain. Xaicrg's gtinpt default is
# NOTSET, so with the read missing addtd/mktdlb build a NON-regime trading-day
# group for the test -- a different design, not a shifted one.
series{
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
  save = (d10 d11 d12 d13)
}
x11regression{
  variables = (td/1955.jan/)
  aictest = (td)
  save = (xrm b16 c16)
}
