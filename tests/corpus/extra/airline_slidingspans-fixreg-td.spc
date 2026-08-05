# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: slidingspans{fixreg=(td)} -- setssp.f:325-336's Ssfxrg decode and
# the Tdfix demote it feeds at ssmdl.f:53-56 (rvfixd on the regARIMA model).
#
# Two things here are load-bearing and must not be "simplified":
#   * the regression{} group -- ssmdl.f:120 zeroes Nssfxr outright when Nb==0,
#     so with no regARIMA regressor at all fixreg= is neutralised by the oracle
#     itself and this spec would measure nothing;
#   * fixmdl = no -- with the default (Ssinit==1) setssp.f:47 demotes Itd to -1
#     before ssmdl is ever called, which hides the fixreg demote behind an
#     identical-looking absence.
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
slidingspans{
  fixmdl = no
  fixreg = (td)
  print = all
  save = (sfs chs tds ads)
}
