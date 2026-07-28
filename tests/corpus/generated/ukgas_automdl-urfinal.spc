# Gates automdl{urfinal=}: the FINAL unit-root threshold. chkrt1 (automd.f:717)
# runs in the label-30 finalization tail; if an AR root's magnitude is under
# this limit a unit root is assumed, the AR order drops by one, the
# differencing order rises by one, and the model is re-estimated.
#
# This is reachable only because the finalization tail now runs on the plain
# (non-aictest) automdl path -- automd.f has ONE path and this driver used to
# reach the tail solely from its aictest branch. At the default 1.05 chkrt1
# finds nothing here; 1.50 makes it bite (measured oracle on-vs-off: 33 .udg
# keys move).
series{
  title  = "UK quarterly gas consumption (NSA)"
  file   = "../data/ukgas.dat"
  start  = 1960.1
  period = 4
}
transform{
  function = log
}
automdl{ urfinal = 1.50 }
estimate{ }
