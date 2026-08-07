# EDGE CASE: a print= name in no dictionary at all. getprt.f tries LVLDIC
# (default none brief alltables all) first, then this spec's slice of
# TB1DIC..TB4DIC; `bogus` is in neither, so the oracle refuses with
# "Print or level argument is not defined." plus a continuation line naming
# both table names AND levels -- the save variant names only table names.
# This port used to consume print= without looking at it.
# EXPECTED TO FAIL.
series{
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
x11{
  print = bogus
}
