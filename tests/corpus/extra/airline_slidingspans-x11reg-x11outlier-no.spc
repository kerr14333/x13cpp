# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: slidingspans{x11outlier = no} with automatic x11regression outlier
# identification -- the wall this port carried until entry 87. The option turns
# OFF per-span re-identification (x11mdl.f:424-425's `Issap.eq.2.and.Ssxotl`)
# and turns ON the hold-back instead: ssxmdl.f:44-77 walks the x11regression
# design, stores every AO column the span INTERSECTION cannot support into
# otxrev, and each span's ssx11a.f:119-151 puts back the ones its own window
# reaches.
#
# THE TABLES HERE ARE BYTE-IDENTICAL TO THE DEFAULT RUN
# (extra/airline_slidingspans-x11reg-autootl), and that is the point of the
# spec, not a reason to drop it: with fixx11reg at its default yes the design
# is fixed and neither route re-estimates anything, so the option is inert on
# the ORACLE too. What the gate holds is that the engine takes the other route
# and lands in the same place -- it used to refuse outright. The arm where the
# two routes DIVERGE is the partner spec -x11outlier-no-fixno (240 sfs lines).
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  print = all
  savelog = all
}
x11regression{
  variables = (td)
  critical = 3.5
  save = (xrm b16 c16)
}
x11{
  print = all
  savelog = all
  save = (d10 d11 d12 d13 d16)
}
slidingspans{
  x11outlier = no
  print = all
  save = (sfs chs tds ads)
}
