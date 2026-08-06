# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: slidingspans{fixreg = (outlier)} -- setssp.f:332-333's Otlfix, which
# unlike tdfix/holfix/usrfix OUTLIVES setssp: sspdrv.f:66-67 declares it as a
# local and passes it to ssx11a once per span (:121), where ssx11a.f:268-269
# re-adds a held-back outlier with `Otlfix.or.Ssinit.eq.1`. Until this spec the
# option was walled, because the port collapsed that disjunction to its fixmdl
# arm.
#
# BOTH slidingspans LINES ARE LOAD-BEARING, and `fixmdl = no` is the one that
# makes the other observable. Measured on the oracle:
#
#   fixreg=(outlier) at the DEFAULT fixmdl=yes  -- byte-inert. Ssinit==1 already
#       makes the disjunction true, so adding the line to
#       extra/airline_slidingspans-outlier-heldback moves not one cell of sfs
#       or chs.
#   fixreg=(outlier) with fixmdl=no             -- 192 of the sfs lines and 190
#       of the chs lines move. With Ssinit==0 the flag is the only thing that
#       can fix a re-added outlier's coefficient, so each span either
#       re-estimates it or does not.
#
# ao1959.nov is the same date as the -heldback and -fixmdlno specs: past the
# span intersection (1954.Jan-1957.Dec), so it is held back and re-added by
# adotss only in spans 3 and 4. ao1950.feb sits before span 1 and takes
# rmotss's delete-outright branch.
#
# tds and ads are saved as well as sfs and chs: with fixmdl=no the Ssinit==1
# Itd/Ihol demote does not fire, so the oracle writes all four tables and the
# calendar-factor pair is gated too.
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
  variables = (ao1950.feb ao1959.nov td)
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
  save = (d10 d11 d12 d13 d16)
}
slidingspans{
  fixmdl = no
  fixreg = (outlier)
  print = all
  save = (sfs chs tds ads)
}
