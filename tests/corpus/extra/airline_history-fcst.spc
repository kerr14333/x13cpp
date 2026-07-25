# coverage: history{estimates=(fcst)} forecast-error history (fce/fch + meanssfe),
# two leads (1 and 12) over a 12-month forecast horizon. Hand-authored; NOT
# produced by genextra.py (which would delete it -- see the corpus-generator
# hazard note in CLAUDE.md).
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
forecast{
  maxlead = 12
}
x11{
  print = all
  savelog = all
}
history{
  estimates = (fcst)
  fstep = (1 12)
  start = 1955.jan
  print = all
  save = (fce fch)
  savelog = all
}
