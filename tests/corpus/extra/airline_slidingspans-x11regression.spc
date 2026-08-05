# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: slidingspans{} together with x11regression{} -- a pairing NO spec in
# the corpus had, which is why slidingspans.cpp asserted "Nbx==0 always in this
# port" and skipped setssp.f:353's ssxmdl on that basis for as long as
# x11regression{} has been ported.
#
# What gates: all four span tables (sfs / chs / ads / tds, all four spans),
# b16/c16, and every D-table -- bit-exact.
#
# ssxmdl is NOT inert here, which is what closed the chs gap this spec was
# landed with. slidingspans{fixx11reg=} defaults to YES (gtinpt.f:531), so
# ssxmdl.f:140-150 fixes every irregular-regression coefficient before the first
# span and each span re-applies the MAIN run's daily weights instead of fitting
# its own. Measured: `fixx11reg=no` moves the per-span TD factor 99.144951 ->
# 98.847324 at 1951.Jan, so the default is load-bearing, not a no-op.
#
# tds only exists because x11mdl.f:874's ssrit is ported -- x11pt2.f:136's is
# keyed on the regARIMA trading day and takes its else-branch on this spec.
# Keep tds and ads in the save list: they are the tables that localise a
# per-span CALENDAR fault, which sfs cannot see.
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
  savelog = all
}
x11{
  savelog = all
  save = (d10 d11 d12 d13 d16)
}
x11regression{
  variables = (td)
  save = (xrm b16 c16)
}
slidingspans{
  print = all
  save = (sfs chs tds ads)
}
