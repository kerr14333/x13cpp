# Hand-authored (2026-08-11). x11regression{defaultcritical=} -> Cvxtyp
# (gtxreg.f:573-578), parsed and DROPPED -- and this is the one of the four that
# already had its READER: entry 93 moved editor.f:1749-1757's Critxr derivation
# into xrg_editor_setup, where `ljung` selects setcvl over setcv. So the option
# was inert only because nothing ever set the flag it is read from.
#
# Sibling of airline_x11regression-outlierspan-default, and it inherits that
# spec's preconditions verbatim: no `critical=` (or editor.f:1749's derivation
# does not run at all) and an `easter[8]` group (or editor.f:1730 takes the
# Sigxrg=2.5 arm and never switches Otlxrg on). Both were rediscovered the hard
# way here -- a first sweep of 80 oracle pairs measured a clean zero because the
# probe specs had `variables=(td)` and no derivation ever happened.
#
# The observable is `x11irrcrtval`: 3.850775 (corrected, the default) against
# 3.848402 (ljung). It does NOT move the adjustment on this spec, and that is
# measured rather than assumed: 8 series spans and 80 series x outlier-span
# pairs on the stock oracle produced a bit-identical d11 every time, because the
# two derivations differ by ~2e-3 to 2e-2 and no AO t-statistic in this corpus
# falls in that window. The savelog value IS the downstream observable here.
#
# NOT produced by gen*.py.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  span = (1951.1,1954.12)
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
  save = (d10 d11 d12 d13)
  savelog = all
}
x11regression{
  variables = (td easter[8])
  defaultcritical = ljung
  print = all
  save = (xrm b16 c16)
}
