# coverage: history{fixmdl=yes} (Revfix) -- hold the WHOLE regARIMA model at the
# main run's converged values for every span, so each span re-FILTERS instead of
# re-estimating (revdrv.f:250-262 + the ssprep re-snapshot at :381 that makes the
# fix survive the per-span restor). Also the `modelspan=(,0.per)` interaction:
# revchk.f:801-805 switches Fixper OFF when the model is fixed anyway.
# Measured oracle on-vs-off (this spec vs the same spec without fixmdl):
#   sae 3.23e-3, tre 3.77e-3, sfe 6.48e-3 relative.
# Because nothing re-optimizes, this is the ONE history configuration that gates
# bit-exact (~5e-15) rather than at the per-span re-estimation floor.
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
  savelog = all
}
history{
  estimates = (sadj sadjchng seasonal trend trendchng)
  start = 1955.jan
  fixmdl = yes
  print = all
  save = (sar sae chr che trr tre tcr tce sfr sfe)
  savelog = all
}
