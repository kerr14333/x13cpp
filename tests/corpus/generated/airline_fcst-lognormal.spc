# coverage: forecast{lognormal=yes} -- the log-normal mean correction on the
# forecasts, prtfct.f:92-103 and :411-415 (lgnrmc.f).
#
# HALF of this was already right, which is what made it invisible: the argument
# was parsed, and the :411 correction to `untfct` (the fct TABLE) was applied.
# The :96 correction to Fcstx -- the forecast X-11 actually EXTENDS THE SERIES
# with, taken on the transformed scale with Ltrans=F -- was not, so the printed
# forecast was right while d10-d13 were wrong. 33 .udg keys move.
#
# Hand-authored; NOT produced by genspecs.py.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{ function = log }
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
forecast{ maxlead = 24  lognormal = yes  save = (fct) }
x11{ save = (d10 d11 d12 d13 d16) }
