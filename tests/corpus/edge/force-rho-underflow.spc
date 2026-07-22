# EDGE CASE: force{ rho= } with a positive value below the dpeq tolerance
# (3.834e-20). getfrc.f rejects it via dpeq(rho, 0). Gates the gt_force rho fix
# (readers_spec.cpp: the old `rho > 0.0` test accepted this; the fix uses the
# getfrc.f form `reject if rho<0 || rho>1 || dpeq(rho,0)`). EXPECTED TO FAIL.
series{
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
x11{ }
force{
  type = regress
  rho  = 1e-25
}
