# Gates automdl{maxdiff=}: the upper bounds on the differencing orders iddiff
# may search, against the (2 1) default. Unlike `diff=` (which FIXES the orders
# and leaves Lautod false, skipping the search) `maxdiff=` keeps the search and
# only caps it, so the two arguments exercise opposite branches of the same
# mdlset/iddiff seam.
#
# ces_leis at (1 0) is the strongest APPLIED case in the corpus: 70 .udg keys
# move, and the seasonal cap of 0 removes the seasonal difference entirely.
# The same value on ukgas/nottem/ces_accfood still DIFFERS -- that residual is
# the unported pass2/nloop re-identification, not this argument; see the
# DEFERRED note in core/src/automdl/automd.cpp.
series{
  title  = "Leisure and hospitality (CES, NSA)"
  file   = "../data/ces_leis.dat"
  start  = 1990.01
  period = 12
}
transform{
  function = log
}
automdl{ maxdiff = (1 0) }
estimate{ }
