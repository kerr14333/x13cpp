# coverage: history{fixx11reg=yes} with NO regARIMA model -- the branch where
# Ixreg is never promoted past 1, so x11pt2's inline x11mdl_td (not the
# transparent xrgdrv pass) is what the fixed columns are struck from. Gates
# BIT-EXACT (~5e-15), unlike the modelled twin, because with no model there is
# nothing to re-estimate per span.
# Hand-authored; NOT produced by gen*.py.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
x11{
  print = all
  savelog = all
}
x11regression{
  variables = (td)
  print = all
}
history{
  estimates = (sadj sadjchng trend trendchng)
  start = 1955.jan
  fixx11reg = yes
  print = all
  save = (sar sae chr che trr tre tcr tce)
  savelog = all
}
