# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: the arm on which ssx11a.f:107-118 is LOAD-BEARING. Same run as
# extra/airline_slidingspans-x11reg-autootl with fixx11reg = no, so each span
# REFITS the irregular regression instead of reloading the main run's fixed
# coefficients -- and therefore runs x11mdl's automatic AO identification
# again.
#
# Before the port of ssx11a.f:99-154 the engine never struck the previous
# span's AO columns, so each span appended a fresh set on top of the last and
# the run died at
#
#   ERROR: Adding AO1953.Feb exceeds the number of regression effects allowed
#          in the model (80).
#
# where the oracle finishes all four spans. That is why this spec exists: the
# feature's whole observable on the DEFAULT arm is nothing at all (see the
# partner spec's header), and a gate written only there would have proved a
# strike that never runs.
#
# On the oracle, 240 of the sfs lines and 226 of the tds lines differ from the
# fixx11reg = yes run.
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
x11regression{
  variables = (td)
  critical = 3.5
  save = (xrm b16 c16)
}
x11{
  print = all
  savelog = all
  save = (d10 d11 d12 d13 d16)
}
slidingspans{
  fixx11reg = no
  print = all
  save = (sfs chs tds ads)
}
