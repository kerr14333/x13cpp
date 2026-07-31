# Hand-authored -- NOT produced by genspecs.py, do not let a regen delete it.
#
# The automdl BASELINE gate for ces_amuse, the series with the narrowest
# best-five gap in the corpus. It exists because for two sessions the engine
# picked a 4-ARMA-term model here against the oracle's 5, which made every
# option verdict measured on this series uninterpretable -- the probe set in
# tools/automdl_scouting.md excluded it for exactly that reason, and the
# sibling ces_amuse_automdl-noautooutlier.spc says so in its own header.
#
# That gap closed when automd.f's adequacy stage stopped being reachable only
# from the aic branch (tools/automdl_scouting.md 3c, UPDATE 2026-07-28c), and
# nobody re-measured it for two more sessions. Re-measured 2026-07-30: the
# oracle's final model is (3 1 1)(0 1 1) and all 105 shared .udg keys agree.
# This spec is here so that can never go stale again -- an exclusion recorded
# in prose is not a gate.
series{
  title  = "Amusements, gambling and recreation (CES, NSA)"
  file   = "../data/ces_amuse.dat"
  start  = 1990.01
  period = 12
}
transform{
  function = log
}
automdl{ }
estimate{ }
