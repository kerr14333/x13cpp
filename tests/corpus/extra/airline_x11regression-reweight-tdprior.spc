# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: the OTHER Lxrneg reader, editor.f:1511 -- with reweight=yes a NEGATIVE
# prior trading-day weight is clamped to zero before the seven are standardized to
# total 7.0. That reader was ported long ago (gtinpt.cpp) and had been reading a
# permanently-false flag ever since.
#
# Log-additive because editor.f:1495 refuses a negative prior weight outright for a
# multiplicative adjustment, and :1507 admits Muladd==2 to the same standardize
# branch. The weights sum to exactly 7.0 as given, so WITHOUT the clamp the scale
# factor is 1 and the -0.5 survives; WITH it the -0.5 becomes 0, the total is 7.5
# and every weight is scaled by 7/7.5. a4 moves 1.2e-2 at 1949.Jan.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
  save = (b1)
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
  mode = logadd
  save = (d10 d11 d12 d13)
  savelog = all
}
x11regression{
  tdprior = (1.4 1.4 1.4 1.4 1.4 -0.5 0.5)
  reweight = yes
  save = (a4)
}
