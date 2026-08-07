# EDGE CASE: the second refusing spec, so the gate is not a claim about
# regression{} alone. slidingspans{}'s savelog slice is `percent`/`pct` and
# `percents`/`pcs` (LSLSSP=58, NSLSSP=2 in svllog.i) -- no `alldiagnostics`, so
# `savelog = all` is refused here too, on a spec that otherwise runs.
# EXPECTED TO FAIL.
series{
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
x11{ }
slidingspans{
  savelog = all
}
