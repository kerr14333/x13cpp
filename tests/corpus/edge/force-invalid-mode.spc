# EDGE CASE: force{ mode= } with a value not in the FMDDIC dictionary
# (ratio|diff). getfrc.f routes mode through gtdcvc; an invalid NAME prints the
# options message AND cascades to "Argument name X not found". Gates the gt_force
# categorical gtdcvc routing (readers_spec.cpp). EXPECTED TO FAIL.
series{
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
x11{ }
force{ mode = sideways }
