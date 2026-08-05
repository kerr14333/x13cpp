# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: x11mdl.f:661-690's STOCK trading-day nonpositive-factor abend. Until
# entry 80 this path was neither ported nor walled: the oracle refused the run
# and the engine returned OUTCOME: OK with a stock TD factor built from
# coefficients that make it nonpositive.
#
# Reaching it needs THE ELSE OF x11mdl.f:546, not of :541 -- an x11regression
# trading day present (Havxtd) with NO "Trading Day" group in the working model,
# which is exactly what a stock design looks like -- plus Muladd==0
# (multiplicative, i.e. no transform{}) and a coefficient <= -1. The b= holds
# the first contrast at -1.5 fixed so the OLS cannot move it.
#
# Its sibling airline_x11regression-tdstock is the same spec with weights that
# stay positive: it must run to OUTCOME: OK. Keep BOTH -- a refusal gated only
# on the firing side cannot tell "correctly refused" from "refuses everything".
series{
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
arima{
  model = (0 1 1)(0 1 1)
}
x11{
  save = (d10 d11)
}
x11regression{
  variables = (tdstock[15])
  b = (-1.5f -0.1f -0.1f -0.1f -0.1f -0.1f)
  save = (xrm b16 c16)
}
