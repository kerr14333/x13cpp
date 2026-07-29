# coverage: pickmdl{outofsample=yes} -- the AUTOMATIC-model arm of the same
# out-of-sample aape (`Outfer`, amdfct.f's `Lauto` path). It was rejected by the
# parser outright while the computation was unported.
#
# This is not a cosmetic switch: the aape is the FIRST of pickmdl's three
# acceptance screens, so changing how it is computed changes which model is
# selected. Measured oracle on-vs-off against airline_pickmdl:
#   arimamdl (0 1 2)(0 1 1) -> (0 1 1)(0 1 1), nmodel 3 -> 2,
#   aape.0 5.5606 -> 5.7552.
#
# `outofsample=` sets Outfer here and Outfct at gtinpt.f:1207-1209; with no
# estimate{outofsample=} beside it the two agree, which is why one argument
# covers both. The mismatched combination is what gtinpt.f:1213-1215 exists for.
#
# Hand-authored; NOT produced by genextra.py, which would delete it.
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
pickmdl{
  file = "pickmdl.mdl"
  method = best
  outofsample = yes
}
estimate{
  savelog = all
}
forecast{
  maxlead = 12
}
x11{
  savelog = all
}
