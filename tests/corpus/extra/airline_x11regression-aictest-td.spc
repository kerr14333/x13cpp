# Hand-authored -- NOT produced by genextra.py, and `python genextra.py` would
# delete it. Do not run that generator.
#
# coverage: x11regression{aictest=(td)} -- the TRADING-DAY branch of x11aic.f
# (:148-295), which the corpus had no spec for at all. The sibling
# airline_x11regression-aictest spec covers only the EASTER branch (:299-458).
# Owns the `aictest.xtd.aicc.{notd,td}` + `aictest.xtd.reg` savelog keys
# (x11aic.f FORMATs 1012/1013).
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
  aictest = (td)
  print = all
  save = (xrm b16 c16)
}
