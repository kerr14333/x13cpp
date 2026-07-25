# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: x11regression{tdprior=} under x11{mode=logadd}. editor.f:1507 allows
# the weights for multiplicative OR log-additive, and x11pt1.f:52 forces Muladd
# 2->0 for the whole prior-adjustment stage -- so logadd takes exactly the same
# divide as mult. run_pre_model had gated its pre-model division on muladd==0 and
# x11pt1's own guard tests muladd AFTER the 2->0 collapse, so logadd fell through
# both: OUTCOME: OK with the prior TD missing from B1 and d10-d13 (~2e-2..3.8e-2,
# exactly a factor of a4). a4 was bit-exact throughout, which is what made it
# invisible -- the factor was computed correctly and then never applied.
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
  print = all
  savelog = all
}
x11{
  mode = logadd
  print = all
  save = (d10 d11 d12 d13)
  savelog = all
}
x11regression{
  tdprior = (1.4 1.4 1.4 1.4 1.4 0.5 0.5)
  print = all
  save = (a4)
}
