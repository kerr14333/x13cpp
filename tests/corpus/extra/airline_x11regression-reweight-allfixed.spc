# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: editor.f:1660-1666 -- with EVERY trading-day coefficient fixed there is
# nothing for the reweighting to rescale, so the oracle writes a NOTE and clears
# Lxrneg for the rest of the run. The run still succeeds; what this pins is that
# the clear happens, i.e. that x11mdl's reweight does NOT fire afterwards.
#
# This is also the spec that made gtxreg.f:861's missing regfix() visible. Irgxfx
# is what :1640 tests, loadxr copies it from Iregfx, and Iregfx was never computed
# for the x11reg design at all -- so this check and editor.f:1675's stock-TD one
# were both reading the regARIMA model's fix state.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
  save = (b1)
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  savelog = all
}
x11{
  save = (d10 d11 d12 d13)
  savelog = all
}
x11regression{
  variables = (td)
  b = (0.1f 0.0f 0.0f 0.0f 0.0f 0.0f)
  reweight = yes
  save = (xrm c16)
}
