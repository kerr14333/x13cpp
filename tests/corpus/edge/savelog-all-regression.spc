# EDGE CASE: `savelog = all` in a spec whose SVLDIC slice has no
# `alldiagnostics`/`all` pair. getsvl.f looks every savelog name up in that
# spec's own slice (svlptr(2*Spcdsp), 2*Nspctb entries) and there is no global
# fallback, so `all` -- legal in estimate/automdl/check/x11/history/spectrum/
# composite/seats -- is an ERROR in regression{}, and in transform{}, pickmdl{},
# outlier{}, x11regression{} and slidingspans{} for the same reason.
# This port used to consume every savelog value without looking at it, so this
# spec returned OUTCOME: OK where the oracle stops.
# EXPECTED TO FAIL.
series{
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
regression{
  variables = (td)
  savelog = all
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
