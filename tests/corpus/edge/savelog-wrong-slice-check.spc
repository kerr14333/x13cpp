# EDGE CASE: a savelog name that is REAL, just not this spec's. `aic` belongs to
# estimate{}'s slice of SVLDIC (LSLEST=8); check{}'s slice (LSLCHK=19) holds
# normalitytest/seasonalacf/ljungboxq/boxpierceq/seasftest/tdftest/durbinwatson/
# friedmantest/alldiagnostics and nothing else, so the oracle refuses it.
#
# This spec exists because the other three could not tell a WRONG slice from the
# right one. `check{}` and `composite{}` were the only two readers that already
# called getsvl, and both passed placeholder displacements -- `getsvl(ctx, 0, 11,
# ...)` -- into a routine that ignored them. Restoring that placeholder fails no
# gate without this spec, because the corpus's only check{} savelog value is
# `all`, which happens to fall inside the placeholder window too (automdl's
# alldiagnostics/all pair sits at entries 11-12).
# EXPECTED TO FAIL.
series{
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
check{
  savelog = aic
}
