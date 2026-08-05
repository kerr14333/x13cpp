# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: the A/B control for slidingspans{fixmdl=}. Identical to
# airline_slidingspans-td.spc except `fixmdl = no`, which is what makes each
# span RE-ESTIMATE the regARIMA model instead of replaying the main run's
# converged one (ssmdl.f:341-352's Ssinit==1 block does not run).
#
# Landed to separate two things that were arriving together: the ssmdl.f:50-121
# regressor-fixing arms all need Ssinit/=1 to be observable at all, so every
# spec that reaches them also carries fixmdl=no -- and this one carries fixmdl=no
# and nothing else.
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
  print = all
  save = (sfs chs)
}
