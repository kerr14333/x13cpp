# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: `x11regression{format=}` (gtxreg.f:624 -> gtfldt.f:71), WALLED at
# core/src/specparse/readers_spec.cpp. Third member of the format= family; see
# edge/airline_series-format-free.spc for the reasoning, and note that this is
# a DIFFERENT gtfldt call site from the regression{} one -- entry 71's rule is
# that a routine correct at one call site is a defect at the next, and the
# whole point of gating each site is that the walls are per-site.
#
# The oracle runs this to completion; the golden proves it.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
x11regression{
  variables = (td)
  user      = (u1 u2)
  usertype  = (user user)
  file      = "../data/userreg2.dat"
  format    = "free"
}
x11{ }
