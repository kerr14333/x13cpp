# Gates automdl{diff=}: FIXED differencing orders, not a search bound.
# The whole effect is that diff= (unlike maxdiff=) leaves Lautod false, so
# automd.f:392 takes its mdlset else-branch instead of calling iddiff. Measured
# on this series: nefobs 104 -> 103, nonseasonaldiff 0 -> 1.
series{
  title  = "UK quarterly gas consumption (NSA)"
  file   = "../data/ukgas.dat"
  start  = 1960.1
  period = 4
}
transform{
  function = log
}
automdl{ diff = (1,1) }
estimate{ }
