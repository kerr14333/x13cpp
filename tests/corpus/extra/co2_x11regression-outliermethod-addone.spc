# Hand-authored (2026-08-11). The CONTROL half of the x11regression
# outliermethod= pair: identical to co2_x11regression-outliermethod-addall
# except that this one leaves the option on its gtinpt.f:460 default (addone).
# The oracle completes here and HALTS on the addall sibling, so the two
# together are what attribute the halt to the option rather than to the series.
#
# critical=3.2 on co2 is not arbitrary: it is the one point of a 154-pair sweep
# (7 series x critical 2.8..4.9) where the two arms of outliermethod= reach
# different verdicts. Everywhere else the forward-addition path differs and the
# backward-deletion step converges on the same design, which is why the whole
# option is invisible in the saved tables -- see the addall sibling's header.
#
# NOT produced by gen*.py.
series{
  title = "Mauna Loa CO2"
  file = "../data/co2.dat"
  start = 1959.01
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
  critical = 3.2
  outliermethod = addone
  print = all
  save = (xrm b16 c16)
}
x11{
  print = all
  save = (d10 d11 d12 d13)
  savelog = all
}
