# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: x11regression{reweight=yes} reaching x11mdl.f:577-612, the daily-weight
# REWEIGHTING itself. `reweight=` (gtxreg.f:553 -> Lxrneg) was parsed and DISCARDED
# while three ported readers took the permanently-false flag as fact.
#
# Reaching the reweight needs a NEGATIVE X-11 daily weight Dx11 = 1 + B. Additive
# mode would make Dx11 = B directly, where any negative TD coefficient suffices, but
# x11pt1's additive prior-TD is walled -- so this fixes five of the six day
# contrasts at 0.39 and lets Saturday be estimated. The derived Sunday coefficient
# comes back at -1.0566, i.e. Dx11(7) = -0.0566 < 0, which is the trigger; Saturday
# lands at -0.8934, so Dx11(6) = 0.1066 is the one positive UNFIXED weight and the
# rescale factor is (7 - 6.95)/0.1066.
#
# The window is narrow and both edges are gated by siblings: below it (v=0.35) the
# Sunday weight stays positive and nothing happens, above it (v=0.43, see
# -reweight-abend) Saturday's own weight goes negative too and the oracle abends.
#
# Discriminating: against -reweight-off (the same spec with reweight=no) 80 c16
# lines and 290 d11 lines differ.
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
  save = (d10 d11 d12 d13 d16)
  savelog = all
}
x11regression{
  variables = (td)
  b = (0.39f 0.39f 0.39f 0.39f 0.39f 0.25)
  reweight = yes
  save = (xrm b16 c16)
}
