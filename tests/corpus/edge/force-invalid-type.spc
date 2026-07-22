# EDGE CASE: force{ type= } with a value not in the FRCDIC dictionary
# (none|denton|regress). getfrc.f routes type through gtdcvc, which on an invalid
# NAME both prints "Entry for type argument must be..." AND leaves the token so
# the dispatch loop reports the paired "Argument name X not found" cascade. Gates
# the gt_force categorical fix (readers_spec.cpp: gtdcvc routing reproduces BOTH
# oracle ERROR lines; the old hand-rolled string compare emitted only the first).
# EXPECTED TO FAIL (3 ERROR lines).
series{
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
x11{ }
force{ type = bogus }
