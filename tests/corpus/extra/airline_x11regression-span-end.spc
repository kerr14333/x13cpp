# Hand-authored (NOT produced by genextra.py).
# coverage: an `x11regression{span=}` that ENDS BEFORE the series span --
# xrgdrv.f:151-158's Xdsp. This is NOT the x11mdl.f:113-118 narrowing that
# airline_x11regression-span-start covers; it is a different mechanism and it
# reaches x11mdl as a POINTER mutation:
#   xrgdrv RE-derives Xdsp from Endspn/Endxrg (overriding editor.f:1976, which
#   leaves it 0 whenever a regARIMA model promoted Ixreg first), pulls
#   Posfob/Posffc back by it, and moves Endspn onto Endxrg WITHOUT touching
#   Nspobs -- so x11mdl's own `nend` comes out ZERO and only its Kpart==3
#   restore (x11mdl.f:515-517) ever sees Xdsp.
#
# Four things this spec is the only thing standing on:
#  - tdset, the END this time. Xnstar/Xn are indexed from the BUFFER; stopping
#    them at the shortened Posffc left the C iteration's last Xdsp factor rows
#    dividing by zero, and the run came back with NaNs in the prior-TD divide.
#    (The START version of the same trap is entry 67.)
#  - the B and C iterations carry DIFFERENT lengths: b16 is 120 rows here and
#    c16 is 144 (x11mdl.f:514-517's `lastpr`), and nfac gains Xdsp at Kpart==3.
#  - the .xrm design matrix is saved at x11mdl.f:499-509, i.e. BEFORE the span
#    restore, so its 132 rows are the NARROW fit's -- not the rebuilt 156.
#  - Nofpob is left DELIBERATELY NARROW. x11mdl.f:126-137 recomputes it off the
#    still-narrow Nspobs and xrgdrv.f:166's restore is conditional, so when the
#    spec's own forecast horizon already equals Nfcstx the narrow value survives
#    into the main run -- the oracle's own B 1 table prints 132 rows over a
#    144-point span. Re-deriving the geometry after xrgdrv discards exactly
#    that, which is why run_pre_model and x11_prestage both skip it here.
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
  span = (1949.01,1958.12)
  print = all
  save = (xrm b16 c16)
}
