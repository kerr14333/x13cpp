# coverage: x11regression{} + history{} in its DEFAULT configuration -- the
# per-span transparent prior-TD pass (revdrv.f:530-532's Ixreg 3->1/2 demote ->
# x11ari.f:88-95's xrgdrv, run against THIS span's pointers).
#
# This was a silent wrong-numbers bug on a default path, not a missing option.
# No corpus spec had ever combined x11regression{} with a span-replay driver
# (checked: zero), so nothing gated it -- the same two-features-at-once blind
# spot as the Priadj restore and the ssprep regression half. With Ixreg left at
# 3 every span reuses the MAIN run's x11regression coefficients, i.e. the engine
# silently behaves as though `fixx11reg=yes` had been given:
#   engine-vs-oracle DEFAULT   sar 8.22e-1, sae 8.27e-3   (tolerances 5e-3 / 1e-5)
#   engine-vs-oracle FIXED     sar 1.28e-3, sae 1.25e-5   (i.e. at the floor)
# With the demote + per-span xrgdrv it is sar 6.8e-4 / sae 6.7e-6, the ordinary
# per-span re-estimation floor.
#
# The estimation input matters as much as the factor: arima.f:156-157 copies the
# X-11 buffer Sto from Pos1ob AFTER x11pt1 has divided out this span's Faccal, so
# run_x11_span rebuilds it that way on this path rather than reusing the main
# run's pre-transformed series (which carries the main run's Faccal).
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
x11regression{
  variables = (td)
  print = all
}
history{
  estimates = (sadj sadjchng trend trendchng)
  start = 1955.jan
  print = all
  save = (sar sae chr che trr tre tcr tce)
  savelog = all
}
