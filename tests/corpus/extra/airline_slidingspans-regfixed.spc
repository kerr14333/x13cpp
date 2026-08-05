# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: ssmdl.f:71-119's `Iregfx.eq.2` arm with EVERY trading-day column
# fixed, so the group walk ANDs down to Tdfix=T and Itd is demoted to -1.
#
# It says Iregfx==2, not ==3, and that is measured rather than assumed: every
# b= value here is fixed, yet getreg.f's Leap Year splice leaves one column
# valueless at regfix.f:31 and the promotion to 3 never happens. Mutation
# testing is what caught the original comment claiming the ==3 arm --
# airline_slidingspans-regallfixed is the spec that actually reaches it.
#
# fixmdl = no is load-bearing: with the default (Ssinit==1) setssp.f:47 already
# demotes Itd, and this arm's own demote would be invisible. Ssinit/=1 is also
# what makes the arm record its verdict in Nssfxr/Ssfxrg.
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
  # Small, plausible log-scale TD coefficients on purpose: at 0.39 the fixed
  # design is so far from the data that the ARMA maximisation runs past 99999
  # iterations and the oracle's udg prints `niter: ***`, which no harness can
  # parse. The arm under test does not care what the values are, only that they
  # are all fixed.
  b = (0.001f 0.001f 0.001f 0.001f 0.001f 0.001f)
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
  print = all
  save = (sfs chs tds ads)
}
