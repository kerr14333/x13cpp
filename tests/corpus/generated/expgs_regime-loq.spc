# Change-of-regime length-of-quarter regression (variables=(loq/date/)).
# Quarterly series exercises the loq regime path (adrgim, PRRTLQ/PRATLQ).
series{
  title = "US Exports (EXPGS)"
  file = "../data/expgs.dat"
  start = 1947.1
  period = 4
}
transform{ function=log }
regression{ variables=(loq/1970.1/) }
arima{ model=(0 1 1) }
estimate{ }
