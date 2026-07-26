# coverage: rmotrv.f / chkorv.f -- the outliers a revisions history HOLDS BACK.
# A span ending at date T must not know about an outlier dated after T, so
# revdrv.f:302 deletes every outlier regressor past the first revision date from
# the design before the loop starts and revdrv.f:589 re-introduces each one when
# a span's MODEL span reaches it. NEITHER IS BEHIND A FLAG -- this is the
# default path, and without it the engine fit every span with the full-series
# outlier set: measured sar 9.8e-1 / sae 9.1e-3 against tolerances of 5e-3 /
# 1e-5, behind an OUTCOME: OK.
# Both regressors here are dated after 1955.01, so both are held back; the
# companion airline_history-outlier-pre pins the other side (an outlier BEFORE
# the start is untouched, and was already correct).
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
regression{
  variables = (ao1957.jan ls1958.jul)
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
history{
  estimates = (sadj sadjchng trend trendchng)
  start = 1955.jan
  print = all
  save = (sar sae chr che trr tre tcr tce)
  savelog = all
}
