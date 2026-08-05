# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: the OTHER arm of adotss's fix flag. ssx11a.f:268-269 re-adds a
# held-back outlier with `Otlfix.or.Ssinit.eq.1`, and every other slidingspans
# spec in the corpus takes fixmdl=yes (the default, Ssinit==1) -- so on all of
# them the disjunction is TRUE and a re-added outlier always comes back FIXED.
# `fixreg=(outlier)`, the only other way to set the flag, is walled.
#
# With fixmdl = no the flag is FALSE: each span re-estimates, and the outlier
# adotss puts back is re-estimated with it. That is the arm no other spec
# reaches, and per this project's rule a mutation that passes is usually a
# claim about which arm the spec is on, not about the code.
#
# ao1959.nov is the same date as extra/airline_slidingspans-outlier-heldback:
# past the span intersection (1954.Jan-1957.Dec), so it is held back and
# re-added only in spans 3 and 4.
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
  variables = (ao1959.nov td)
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
  fixmdl = no
  print = all
  save = (sfs chs)
}
