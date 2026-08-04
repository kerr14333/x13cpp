# Hand-authored (NOT produced by genextra.py).
# coverage: gtinpt.f:832's `restor(T,F,F)` -- restor.f:73 `Picktd=Pktd2` -- and
# what reads Picktd afterwards, gtinpt.f:999-1032.
#
# `variables=(td)` inside x11regression{} sets Picktd through the same
# adpdrg.f:642 line the regARIMA parser uses. gtxreg then parks the x11reg
# model in the side store (loadxr.f:53 `Pckxtd=Picktd`, which is where xrgdrv
# and x11mdl read it), and gtinpt.f:832 puts the WORKING flag back to the
# gtinpt.f:804 snapshot. This port's stand-in for that restor cleared the
# regressors and left Picktd set, so gtinpt.f:999-1032 -- which runs AFTER
# gtxreg (line 999 > 832) and only under `dpeq(Lam,ZERO)` -- called rmlnvr and
# turned an x11regression-only `td` into a leap-year PRIOR on the series
# (Priadj=4, Kfmt=1) that the oracle never applies.
#
# So the trigger needs all three: a log transform (the Lam gate), a `td` that
# lives ONLY in x11regression{}, and no regARIMA model to mask it. Measured
# before the fix: d11 1.2e-2, d13 1.6e-2, b16/c16 1.5e-3, at OUTCOME: OK.
#
# The invariant this pins is the one the oracle actually has: with no model,
# `transform{function=log}` is a NO-OP for every X-11 table. This spec and
# airline_x11regression-nomodel-priortd differ by `transform{}` plus `span=`,
# and the log/no-log pair is bit-identical in the oracle.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
}
x11{
  print = all
  savelog = all
  save = (d10 d11 d12 d13 d16)
}
x11regression{
  variables = (td)
  print = all
  save = (xrm b16 c16)
}
