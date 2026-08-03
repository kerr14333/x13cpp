# Hand-authored (NOT produced by genextra.py).
# coverage: an `x11regression{span=}` that narrows BOTH ends at once, which is
# the only place the two DIFFERENT narrowing mechanisms have to compose:
#   - the START is x11mdl.f:113-118 -- Begspn/Nspobs move inside x11mdl and
#     :512-528's setspn + regvar rebuild put them back (`nbeg > 0`);
#   - the END is xrgdrv.f:151-158 -- Xdsp shortens Posfob/Posffc across the
#     whole transparent pass and moves Endspn, so x11mdl's `nend` is ZERO and
#     the restore comes from :515-517 instead.
# Together they mean the Kpart==3 restore runs with nbeg > 0 AND nend = Xdsp,
# and the guard that disarms the SpanGuard has to leave the result alone --
# the C iteration ENTERS with the B iteration's still-narrow Nspobs, so an
# unconditional restore-to-entry would undo it.
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
  span = (1951.07,1958.12)
  print = all
  save = (xrm b16 c16)
}
