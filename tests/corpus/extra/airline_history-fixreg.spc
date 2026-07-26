# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: history{fixreg=(td)} (Rvfxrg) -- revdrv.f:112-134's group decode +
# rvfixd.f. The named regression GROUPS are held at the main run's converged
# values for every span; unlike fixmdl the ARMA parameters still re-estimate, so
# this rides the ordinary per-span re-estimation floor rather than being exact.
#
# Measured oracle on-vs-off before porting (tools/history_options_scouting.md):
# sar 6.3e+2, sae 6.9e-3 -- the flag was parsed, stored, and never read, so the
# engine returned OUTCOME: OK with the "off" numbers.
#
# This spec ALSO pins arima.f:283's rmfix(...,1), which had no call site
# anywhere in the port: without it the fixed flags are set and the estimator
# ignores them entirely, so the run comes back identical to the no-fixreg case.
#
# transform=log + regression{td} is required, not incidental: it is what creates
# the automatic length-of-month/leap-year prior whose per-span handling the
# Priadj restore (ssprep.f:56-62 / restor.f:55) governs.
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
  variables = (td)
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
  fixreg = (td)
  print = all
  save = (sar sae chr che trr tre tcr tce sfr sfe)
  savelog = all
}
