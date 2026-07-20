# Minimal fixed-transform airline spec to exercise the automatic differencing-
# order identifier (iddiff.f) in isolation. Log transform, no regression, so
# iddiff runs on the clean log series -- the canonical airline result is (1,1)
# (idnonseasonaldiff.first / idseasonaldiff.first in the automdl .udg).
series{
  title  = "International Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
}
