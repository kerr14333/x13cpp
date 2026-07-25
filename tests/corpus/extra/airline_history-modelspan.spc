# coverage: history{} with a plain series{modelspan=} FIXED END. revdrv.f:490-496
# caps every span's Endmdl at `mdl2` (the main run's Endmdl) whenever the span
# runs past it -- so from 1959 on, each span's regARIMA model is still estimated
# only through 1958.dec while X-11 sees the whole span. The Fixper sibling
# (airline_history-fixper) is the "0.per" branch of the same block.
# Measured oracle on-vs-off (this spec vs the same spec with no modelspan):
#   sae 7.17e-4, tre 1.97e-3 relative.
# Hand-authored; NOT produced by gen*.py.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
  modelspan = (1949.01, 1958.dec)
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
history{
  estimates = (sadj sadjchng seasonal trend trendchng)
  start = 1955.jan
  print = all
  save = (sar sae chr che trr tre tcr tce sfr sfe)
  savelog = all
}
