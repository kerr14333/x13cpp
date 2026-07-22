# EDGE CASE: force{ round= } with a value not in the YSNDIC dictionary (yes|no).
# getfrc.f routes round through gtdcvc; an invalid NAME prints the options message
# AND cascades to "Argument name X not found". Gates the gt_force categorical
# gtdcvc routing (readers_spec.cpp: the old path accepted any non-"yes" as "no",
# silently swallowing invalid values). EXPECTED TO FAIL.
series{
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
x11{ }
force{ round = maybe }
