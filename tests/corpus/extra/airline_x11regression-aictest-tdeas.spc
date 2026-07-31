# Hand-authored -- NOT produced by genextra.py, and `python genextra.py` would
# delete it. Do not run that generator.
#
# BOTH x11aic.f tests in one run, which is the only way to reach the coupling
# between them: when trading day is ACCEPTED, x11aic.f:242 sets estend=F and
# :244 copies aictd into aichol, so the Easter loop's first iteration (:327)
# skips its own estimation entirely. The observable consequence is that
# `aictest.xe.aicc.noeaster` must come out bit-identical to
# `aictest.xtd.aicc.td` -- a shared value neither single-test spec can show.
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
estimate{
  print = all
  savelog = all
}
x11{
  print = all
  savelog = all
}
x11regression{
  variables = (td)
  aictest = (td, easter)
  print = all
  save = (xrm b16 c16)
}
