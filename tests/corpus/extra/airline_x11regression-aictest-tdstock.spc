# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: editor.f:1802-1808, the ONLY writer of Xaicst -- the day-of-month a
# stock trading-day regressor is measured on, parsed back out of the group
# TITLE ("Stock Trading Day[15]") rather than carried from the spec.
#
# Two things are load-bearing:
#   * tdstock[15], not the default. Xaicst's gtinpt default is 31, so a spec
#     using [31] would agree whether the read happens or not -- the
#     inert-at-its-default trap. 15 is a value nothing else can produce.
#   * aictest = (td), not (tdstock). editor.f:1763-1764 rewrites Xtdtst 1 -> 2
#     when a stock group is present, so this also pins that rewrite; asking for
#     tdstock directly would arrive at 2 without it.
series{
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  savelog = all
}
x11{
  savelog = all
  save = (d10 d11 d12 d13)
}
x11regression{
  variables = (tdstock[15])
  aictest = (td)
  save = (xrm b16 c16)
}
