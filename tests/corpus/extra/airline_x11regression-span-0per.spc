# Hand-authored (NOT produced by genextra.py).
# coverage: gtxreg.f:636-642 -- the `0.per` form of `x11regression{span=}`.
#
# `span = (start, 0.per)` means "end at period `per` of the last year", and it
# is the ONLY writer of Fxprxr, which revdrv.f:500-503 re-derives the span end
# from on every history iteration and editor.f:1976 promotes Ixreg on. Neither
# Fxprxr nor the span itself had a writer in this port.
#
# per==12 on a series ending in December resolves to the series end, so nothing
# narrows and the run is gateable; `0.12` on a series ending earlier, or a
# narrower explicit span, is refused (x11mdl.f:115-118, docs/WALLS.md).
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
  save = (d10 d11 d12 d13)
}
x11regression{
  variables = (td)
  span = (1949.01,0.12)
  print = all
  save = (xrm b16 c16)
}
