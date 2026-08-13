# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: the ORDER of spcrsd's residual-peak WARNING against an X11PT3-phase
# message. This is the pairing entry 109 could not find anywhere in the corpus,
# and without it the residual spectrum can be computed in the ESTIMATION phase
# (where arima.f:1126 calls spcrsd) or late, with the other three spectra, and
# every one of the 525 `.err` goldens comes out the same either way.
#
# The oracle writes, in this order:
#   WARNING ... spectrum of the regARIMA residuals   <- arima.f:1126, estimation
#   NOTE: Negative values were created ...           <- x11pt3.f:647
#   NOTE: Values <= 0 ... forced ...                 <- x11pt3.f:796
#   NOTE: Values <= 0 ... final forced ...           <- x11pt3.f:863
#   WARNING ... one or more of the estimated spectra <- spcdrv.f:594, after x11
# Compute the residual block late and the first line lands with the last.
#
# Both halves are borrowed, not invented. The negative-value NOTEs come from
# `airline_force-constant-denton`, unchanged: airline_zero crosses zero, so
# removing transform{constant=} drives the SA series below it. What differs is
# the MODEL -- dropping the seasonal MA leaves the seasonality in the residuals,
# and the residual spectrum then carries a visually significant seasonal peak
# where the (0 1 1)(0 1 1) sibling reaches only 5.8 of the 6.0 it needs.
series{
  title = "Airline, shifted to cross zero"
  file = "../data/airline_zero.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
  constant = 100
}
arima{
  model = (0 1 1)(0 0 0)
}
estimate{
  savelog = all
}
x11{
  save = (d10 d11 d12 d13 d16)
  savelog = all
}
force{
  type = denton
  save = (saa ffc)
}
