# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: `transform{format=}` (getadj.f:532 -> gtfldt.f:71), WALLED at
# core/src/specparse/readers_spec.cpp. Fourth and last reachable member of the
# format= family; see edge/airline_series-format-free.spc for the reasoning.
# The fifth gtfldt call site, gtxreg.f:810's umfile= read, sits behind the
# Haveum wall and cannot be reached from a spec.
#
# The prior factors are the same 0.97..1.03 cycle the generated
# airline_user-permprior-x11.spc carries inline, moved to a file so that the
# file= / format= path is what runs.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{
  function = log
  type     = permanent
  file     = "../data/priorfac.dat"
  format   = "free"
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
x11{ }
