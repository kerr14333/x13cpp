# coverage: history{} with Fixper -- series{modelspan=( ,0.jan)}, the "0.per"
# convention. rev.cmn: "Period every year for which the model will be estimated
# in the revisions history. Every other period, the model parameters will be
# fixed to what they were at the last value of Fixper." revdrv.f:481-489 caps
# each span's Endmdl at the last January at or before the span end, so the
# estimation window only advances once a year; setrvp.f:64-71 backs Beglup up to
# the first such January.
# Measured oracle on-vs-off (this spec vs the same spec with no modelspan):
#   sae 2.09e-3, tre 3.73e-3, sfe 1.09e-2 relative.
# Hand-authored; NOT produced by gen*.py.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
  modelspan = ( , 0.jan)
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
