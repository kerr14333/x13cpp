# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: slidingspans{} together with AUTOMATIC x11regression outlier
# identification (x11regression{critical=} -> Otlxrg). No corpus spec paired
# them before, which is how ssx11a.f:99-154 -- the x11regression half of the
# per-span outlier bookkeeping, inside the loadxr(F)/loadxr(T) swap -- stayed
# both unported and unwalled.
#
# This is the DEFAULT arm: slidingspans{x11outlier=} defaults to yes, so
# ssx11a.f:107-118 strikes the previous span's automatically identified AO
# columns and x11mdl re-identifies from a clean design in each span. On this
# spec the strike is INERT (the engine matched the oracle before it was
# ported), because fixx11reg= also defaults to yes and a fixed design is not
# refit. Its partner spec -x11reg-autootl-fixno is the same run with
# fixx11reg = no, where the same strike is the difference between finishing and
# dying at the 80-regressor limit -- see docs/M5_PORT_NOTES.md entry 87.
#
# critical = 3.5 is chosen, not default: at 2.5 the oracle itself refuses
# ("Adding AO1957.Mar exceeds the number of regression effects allowed in the
# model (80)"), and 3.5 leaves 7 automatically identified AOs -- enough that
# every span's design differs from the main run's.
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
  print = all
  save = (sfs chs tds ads)
}
