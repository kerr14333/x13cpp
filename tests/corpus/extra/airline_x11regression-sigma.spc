# coverage: x11regression{sigma=} (Sigxrg) -- the sigma limit tdxtrm uses to
# exclude extreme irregulars from the trading-day regression. Parsed and
# silently DROPPED alongside critical=: the port hardcoded editor.f:1729-1736's
# 2.5 default, so an explicit sigma= did nothing. Measured on the oracle,
# sigma=2.0 vs the 2.5 default moves d11 by 1.6e-1 (123.2706 vs 123.1133 at
# 1949.Jan), and the engine now matches bit-exactly.
#
# Note this is the OTHER branch of editor.f:1729-1736 from
# airline_x11regression-critical: sigma= and critical= are alternatives (an
# explicit critical= switches on outlier identification and leaves Sigxrg at 0,
# skipping tdxtrm entirely), so both are needed to cover the rule.
# Hand-authored; NOT produced by gen*.py.
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
  save = (d10 d11 d12 d13)
  savelog = all
}
x11regression{
  variables = (td)
  sigma = 2.0
  print = all
  save = (xrm b16 c16)
}
