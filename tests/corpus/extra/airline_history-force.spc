# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: x11pt3.f:828-831 -- with force{} on (Iyrt>0) the revisions-history SA
# store takes the FORCED series Stci2 and the Iyrt==0 site at :684 never runs.
# The sliding-spans twin of this is airline_slidingspans-force; the history one
# had no carrier, and until getrev moved inside x11pt3 this port re-read
# ctx.x11srs.stci after the pass -- the UNFORCED D11 -- for every row of the sar/
# sae/chr/che tables.
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
  save = (d10 d11 d12 d13 d16)
  savelog = all
}
force{
  type = denton
  print = all
  save = (saa ffc)
}
history{
  estimates = (sadj sadjchng trend trendchng)
  start = 1956.jan
  print = all
  save = (sar sae chr che trr tre tcr tce)
  savelog = all
}
