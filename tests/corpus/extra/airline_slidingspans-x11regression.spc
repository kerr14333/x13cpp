# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: slidingspans{} together with x11regression{} -- a pairing NO spec in
# the corpus had, which is why slidingspans.cpp asserted "Nbx==0 always in this
# port" and skipped setssp.f:353's ssxmdl on that basis for as long as
# x11regression{} has been ported.
#
# What gates: sfs (all four spans), b16/c16, and every D-table -- bit-exact.
# ssxmdl is inert on this spec (no x11regression{span=}, nothing fixed,
# Irgxfx==1), so the claim turned out to be harmless HERE; it was never checked,
# which is the finding.
#
# What does NOT gate: chs. It is a KNOWN GAP recorded in
# tests/parity/test_slidingspans_tables.py, and it is a DIFFERENT gap from
# airline_slidingspans-td's chs despite the identical symptom -- deleting
# x11regression{} from this spec makes chs bit-exact, while deleting
# transform{function=log} (the lom/leap-year prior the other gap is about)
# changes nothing. sfs bit-exact + chs wrong means the per-span seasonal factors
# are right and the per-span calendar factor is not. The golden is blessed and
# committed, so whoever closes it has the target.
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
  save = (sfs chs)
}
