# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: revdrv.f:416-427 -- a history span PAST Endsa (and not the final
# one) runs with Lx11/Lseats FALSE: estimated for the model diagnostics, no
# adjustment produced, nothing captured. This port ran X-11 on every span and
# simply did not read those back, which stopped being equivalent the moment
# getrev moved inside x11pt3.
#
# It takes BOTH options to reach it. `endtable=` alone leaves endsa < endrev but
# every row a late span could write is past Revnum and unread; `sadjlags=` alone
# widens Endsa past the table so no span is late. Together, a span past Endsa
# would file the "12 later" estimate for a row the table DOES print
# (getrev.f:58-71's Finsa(i,i1)) -- which is exactly what the oracle's Lx11=F
# prevents. Neither of the two existing specs (airline_history-endtable,
# airline_history-sadjlags) discriminates the guard; this one does.
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
  endtable = 1957.dec
  sadjlags = (12)
  trendlags = (12)
  print = all
  save = (sar sae chr che trr tre tcr tce)
  savelog = all
}
