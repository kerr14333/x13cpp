# Hand-authored -- NOT produced by genextra.py, and `python genextra.py` would
# delete it. Do not run that generator.
#
# The REJECT arm of x11aic.f's trading-day test (:248-294), which the sibling
# airline_x11regression-aictest-td spec cannot reach: there the AICC gap is
# 7.51 in favour of trading day, so an aicdiff above that flips the verdict and
# drives the branch that deletes the TD columns, clears Axrgtd/Havxtd and
# rebuilds the design without them. Same trick, same reason, as
# airline_x11regression-aicdiff does for the Easter half.
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
  aicdiff = 10.0
  print = all
  save = (xrm b16 c16)
}
