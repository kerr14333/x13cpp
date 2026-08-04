# Hand-authored (NOT produced by genextra.py -- do not delete when regenerating).
# coverage: the AICtd test on a HOLIDAY-ONLY irregular regression -- the case the
# board carried for two sessions as "the auto-AO AICC gap".
#
# It was misdiagnosed twice, and both misdiagnoses are worth keeping:
#
#  1. The AICCs came out ~7.9 apart, and the reading was that the AIC baseline
#     must be fitted on an AUTOMATIC-AO design (editor.f:1727 picks Otlxrg over
#     Sigxrg=2.5 whenever there is no TD group, and the oracle's .out does add
#     AO1960.Mar at t=-5.50). Wrong: x11mdl.f calls x11aic at :253 and does the
#     AO identification at :424, so the aictest baseline never sees an AO design.
#  2. It was recorded as NOT SEPARABLE from the CB-37 Grpx(-1) flip, because
#     x11regression demands a trading-day OR holiday regressor, so "no TD group"
#     forces a holiday. True as far as it goes -- but the AICC gap had nothing to
#     do with the aictest. Dropping `aictest=` reproduced it exactly (see
#     airline_x11regression-holiday-only), which is what finally located it: the
#     whole xrgdrv prior pass was being skipped for a holiday-only design.
#
# Both readings were reasoned from the Fortran without measuring the plain
# holiday-only case first. The engine-vs-oracle delta was measured; the
# CHEAPER-SPEC-vs-CHEAPER-SPEC delta was not.
#
# What this spec pins that the no-aictest sibling does not: `aictest.xtd.reg`
# (td1coef, via the CB-37 alias), `aictest.xtd` (no), and the two AICCs
# themselves -- aictest.xtd.aicc.notd -736.359339510335 and
# aictest.xtd.aicc.td -1732.14491025233, which were -744.271419570893 and
# -1740.01162137319 before the prior pass ran.
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
x11{
  print = all
  savelog = all
  save = (d10 d11 d12 d13 d16)
}
x11regression{
  variables = (easter[8])
  aictest = (td)
  print = all
  save = (xrm)
}
