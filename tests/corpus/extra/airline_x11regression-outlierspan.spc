# coverage: x11regression{outlierspan=} (gtxreg.f:487-497 -> Begxot/Endxot at
# :665-674) -- the window the AUTOMATIC AO identification inside the irregular
# regression searches. Start-date-only form, so the end falls back to the
# default.
#
# Parsed and DISCARDED until now: x11reg.cpp re-derived begxot/endxot locally
# from Begspn/Nspobs on every call, so the option was silently ignored on the
# main run. Measured on this series: the oracle identifies 203 AO columns over
# the full span and 9 with the line below, and this engine returned the
# unrestricted 203 at OUTCOME: OK.
#
# Hand-authored; NOT produced by gen*.py.
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
  save = (d10 d11 d12 d13)
  savelog = all
}
x11regression{
  variables = (td)
  outlierspan = (1955.1, )
  critical = 3.0
  print = all
  save = (xrm b16 c16)
}
