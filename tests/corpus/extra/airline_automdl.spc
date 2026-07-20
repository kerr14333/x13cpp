# Minimal fixed-transform airline automdl spec to exercise the automatic ARMA-
# order identifier (amdid.f) after iddiff. Log transform, no regression, bare
# automdl{} -- so gt_automdl applies the default maxorder=(2,1)/maxdiff=(2,1).
# The canonical airline result is (0 1 1)(0 1 1).
series{
  title  = "International Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{
  function = log
}
automdl{ }
