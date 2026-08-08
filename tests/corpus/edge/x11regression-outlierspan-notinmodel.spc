# EDGE CASE: the SECOND of gtxreg.f:678-695's coverage checks, and the reason
# the first one is not enough. This outlier span lies entirely inside the
# series, so chkcvr(Begsrs,Nobs,...) passes; it is the irregular regression's
# own `span=` it falls outside of, which is a different pair of dates and a
# different message ("Span not within the model span", against 'Model span'
# rather than 'Series'). A port that wired only the first arm gates green here.
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
  span = (1955.1,1958.12)
  outlierspan = (1950.1,1958.12)
  critical = 3.0
}
