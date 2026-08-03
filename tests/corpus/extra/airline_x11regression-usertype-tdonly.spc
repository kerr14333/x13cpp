# Hand-authored (NOT produced by genextra.py).
# coverage: gtxreg.f:833-847 -- `usertype=` is refused when NO column landed in
# a user group.
#
# A lone `usertype=(td)` column is titled 'User-defined Trading Day', so neither
# 'User-defined' nor 'User-defined Holiday' exists and the oracle rejects the
# spec. This engine ran it to OUTCOME: OK -- the port's most dangerous shape.
#
# Note what it does NOT refuse: the same column also sets Havxtd, so Axrgtd is
# true and gtxreg.f:891's td-or-holiday requirement (the sibling
# -user-notd spec) is satisfied. These two refusals are independent.
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
  save = (d10 d11 d12 d13 d16)
  savelog = all
}
x11regression{
  user = (u1)
  usertype = (td)
  data = (
    0.050000 0.049384 0.047553 0.044550 0.040451 0.035355
    0.029389 0.022700 0.015451 0.007822 0.000000 -0.007822
    -0.015451 -0.022700 -0.029389 -0.035355 -0.040451 -0.044550
    -0.047553 -0.049384 -0.050000 -0.049384 -0.047553 -0.044550
    -0.040451 -0.035355 -0.029389 -0.022700 -0.015451 -0.007822
    0.000000 0.007822 0.015451 0.022700 0.029389 0.035355
    0.040451 0.044550 0.047553 0.049384 0.050000 0.049384
    0.047553 0.044550 0.040451 0.035355 0.029389 0.022700
    0.015451 0.007822 0.000000 -0.007822 -0.015451 -0.022700
    -0.029389 -0.035355 -0.040451 -0.044550 -0.047553 -0.049384
    -0.050000 -0.049384 -0.047553 -0.044550 -0.040451 -0.035355
    -0.029389 -0.022700 -0.015451 -0.007822 0.000000 0.007822
    0.015451 0.022700 0.029389 0.035355 0.040451 0.044550
    0.047553 0.049384 0.050000 0.049384 0.047553 0.044550
    0.040451 0.035355 0.029389 0.022700 0.015451 0.007822
    0.000000 -0.007822 -0.015451 -0.022700 -0.029389 -0.035355
    -0.040451 -0.044550 -0.047553 -0.049384 -0.050000 -0.049384
    -0.047553 -0.044550 -0.040451 -0.035355 -0.029389 -0.022700
    -0.015451 -0.007822 0.000000 0.007822 0.015451 0.022700
    0.029389 0.035355 0.040451 0.044550 0.047553 0.049384
    0.050000 0.049384 0.047553 0.044550 0.040451 0.035355
    0.029389 0.022700 0.015451 0.007822 0.000000 -0.007822
    -0.015451 -0.022700 -0.029389 -0.035355 -0.040451 -0.044550
    -0.047553 -0.049384 -0.050000 -0.049384 -0.047553 -0.044550
    -0.040451 -0.035355 -0.029389 -0.022700 -0.015451 -0.007822
    0.000000 0.007822 0.015451 0.022700 0.029389 0.035355
  )
  start = 1949.01
  print = all
  save = (xrm b16 c16)
}
