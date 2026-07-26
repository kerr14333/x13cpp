# coverage: history{sadjlags= trendlags=} -- the ALTERNATE REVISION TARGETS.
# Each lag adds one column to every table of its family: "the estimate `lag`
# periods after the revision date" in place of the full-data final one
# (getrev.f:57-70/86-99 -> prtrev.f:174-226). Both lists carry the 1-year and
# 2-year pair, so revchk.f:1077 sets Lr1y2y and BOTH families additionally get
# the (1yr-2yr) column -- see CB-22 for why the pair has to be given to both.
# Rows whose target estimate does not exist yet are DNOTST (-999) in the save
# file, which this gate compares literally.
# setrvp.f:26-40 widens Endsa by the largest lag so those spans actually run.
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
  estimates = (sadj sadjchng trend trendchng)
  start = 1955.jan
  sadjlags = (12 24)
  trendlags = (12 24)
  print = all
  save = (sar sae chr che trr tre tcr tce)
  savelog = all
}
