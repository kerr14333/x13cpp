# Hand-authored (NOT produced by genextra.py).
# coverage: gtxreg.f:629-661 -- `x11regression{span=}` resolved into
# Begxrg/Endxrg.
#
# Both were unwritten and `span=` fell through to consume_value, so the option
# was parsed and DISCARDED. Measured on this series, the oracle honours it: an
# explicit narrower span moves d11 1.3e-2 and the `0.per` form 1.6e-2, while
# this engine returned the unrestricted answer at OUTCOME: OK.
#
# This spec names the FULL series span, so nbeg==nend==0 and no narrowing is
# required -- which is what makes it gateable. It pins the derivation itself
# (the NOTSET defaults, the chkcvr coverage check, Fxprxr staying 0 and
# editor.f:1976's Xdsp coming out 0), and it pins that a span equal to the
# series span does NOT trip the narrowing wall. A span that actually narrows is
# refused (x11mdl.f:115-118, docs/WALLS.md).
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
  save = (d10 d11 d12 d13)
}
x11regression{
  variables = (td)
  span = (1949.01,1960.12)
  print = all
  save = (xrm b16 c16)
}
