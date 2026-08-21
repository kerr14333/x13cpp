# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: `regression{format=}` (getreg.f:560 -> gtfldt.f:71), WALLED at
# core/src/specparse/readers_spec.cpp. Same family and same shape as
# edge/airline_series-format-free.spc; read that spec's header first.
#
# Base is generated/airline_user-reg-file.spc plus one line, `format = "free"`,
# so the ORACLE's read is identical to the free-format path and the golden is a
# complete successful run. That is the evidence the wall is a GAP.
#
# edge/, not extra/ -- the table gates discover from tests/golden/extra and
# assert the harness exited 0, which a spec whose subject is a refusal cannot.
#
# Gated by test_err_block.py::_ENGINE_WALLS. Excluded from
# test_m1_parse's outcome comparison via _ENGINE_WALL_SPECS, for the reason
# spelled out there.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{
  function = log
}
regression{
  user   = (u1 u2)
  file   = "../data/userreg2.dat"
  format = "free"
}
arima{
  model = (0 1 1)
}
estimate{ }
