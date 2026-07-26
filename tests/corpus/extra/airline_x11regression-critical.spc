# coverage: x11regression{critical=} (Critxr) -- the AO critical value for the
# automatic outlier identification in the IRREGULAR regression, which the
# argument also switches on (gtxreg.f:311-321 sets Otlxrg).
#
# Both this and x11regression{sigma=} were accepted by the parser and silently
# DROPPED: core/src/x11/x11reg.cpp derived the critical value from the span
# length unconditionally (setcv) and hardcoded the 2.5-sigma tdxtrm limit, so
# `critical = 3.0` identified the DEFAULT's outlier set (critical ~3.89) rather
# than the user's -- d11 off 1.7e-4 relative behind an OUTCOME: OK.
#
# It also blocks history{x11outlier=}: that flag only does anything once the
# x11regression design carries automatically identified outliers.
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
  critical = 3.0
  print = all
  save = (xrm b16 c16)
}
