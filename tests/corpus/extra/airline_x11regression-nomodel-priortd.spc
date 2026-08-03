# Hand-authored (NOT produced by genextra.py).
# coverage: x11ari.f:88-95 -- the xrgdrv OLS prior-TD call on the NO-MODEL path.
#
# `IF(Ixreg.eq.2.or.Khol.eq.1) CALL xrgdrv(Lmodel,...)` sits under `IF(Lx11)`
# alone: the oracle reaches it whether or not a regARIMA model was requested,
# and Lmodel only selects how wide xrgdrv's own ssprep/restor snapshot is. This
# port hoists the call into run_pre_model, which runs only when there IS a
# model, so a no-model spec that promotes Ixreg used to skip the transparent
# prior-TD pass entirely and answer as if Ixreg were 1. It was walled rather
# than silent; the wall is now gone and the call is issued from x11_prestage at
# x11ari's own point in time (after x11int, before x11pt1).
#
# The promotion route here is editor.f:1976-1978: `span = (start, 0.per)` is the
# only writer of Fxprxr, and `Khol>=1 .or. Fxprxr>0 .or. Xdsp>0` lifts Ixreg
# 1->2. `0.12` on a series ending in December resolves to the series end, so
# Xdsp is 0 and nothing narrows -- the span form is here to trigger the
# promotion, not to exercise the narrowing (that is the -span-end spec).
#
# Measured: the oracle moves d10 9.2e-3 / d11 8.3e-3 / d12 8.4e-3 / d13 1.6e-2
# against the same spec without `span=`, so the pass is not cosmetic.
#
# There is deliberately NO transform{} here. With no model the oracle's
# `transform{function=log}` is a no-op for every X-11 table (measured: the
# log and no-transform runs are bit-identical), but this engine is NOT yet
# bit-identical across that pair on the x11regression route -- ~1.2e-2 on d11,
# tracked separately. Keeping the transform out isolates what this spec is for.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
x11{
  print = all
  savelog = all
  save = (d10 d11 d12 d13 d16)
}
x11regression{
  variables = (td)
  span = (1949.01,0.12)
  print = all
  save = (xrm b16 c16)
}
