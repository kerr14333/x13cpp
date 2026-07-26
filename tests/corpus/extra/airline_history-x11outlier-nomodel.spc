# coverage: the MODEL-FREE twin of airline_history-x11outlier -- no transform{},
# arima{} or estimate{}, so nothing re-estimates a regARIMA model per span and
# the only thing moving is the per-span x11regression outlier identification.
# revdrv.f:336-338 and :601-603 delete the automatically identified x11regression
# outliers at the head of the analysis and again at every span head, so each span
# re-identifies its own on its own data (loadxr(false) swaps the x11reg store
# into the working model, rmatot strikes the auto columns, loadxr(true) saves it
# back -- nothing restores that store between spans, which is what makes it
# stick).
#
# Ungateable until this session's x11regression{critical=} fix: without it the
# x11reg design carried no automatic outliers at all, so the blocks were inert
# and the whole family measured the MAIN-run gap instead (sar 5.2e-1).
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
  critical = 3.0
  print = all
}
history{
  estimates = (sadj trend)
  start = 1955.jan
  print = all
  save = (sar sae trr tre)
  savelog = all
}
