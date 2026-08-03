# Hand-authored (NOT produced by genextra.py).
# coverage: the `0.per` form of `x11regression{span=}` (gtxreg.f:636-642) with a
# start that actually narrows -- so unlike the sibling -span-0per spec, the
# resolved span differs from the series span and the `0.per` arm is doing real
# work. Fxprxr is set here too.
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
  span = (1952.01,0.12)
  print = all
  save = (xrm b16 c16)
}
