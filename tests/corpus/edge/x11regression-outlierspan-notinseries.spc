# EDGE CASE: the first of gtxreg.f:678-695's two coverage checks -- an outlier
# test span that starts before the series does. It pins TWO things the port did
# not have: the check itself (the option was parsed and dropped, so nothing was
# ever checked), and `cvrerr.f`, the DETAIL lines every failed chkcvr in this
# program is followed by. The port had the inpter half at all sixteen call
# sites and none of the second half, so a coverage refusal came out one third
# the size the oracle writes it.
# EXPECTED TO FAIL.
series{
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
arima{
  model = (0 1 1)(0 1 1)
}
x11{ }
x11regression{
  variables = (td)
  outlierspan = (1940.1, )
  critical = 3.0
}
