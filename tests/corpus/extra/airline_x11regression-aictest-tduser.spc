# Hand-authored (NOT produced by genextra.py).
# coverage: x11aic.f:462-591 -- the USER-defined AICC branch (aictest.xu*), the
# last of x11aic's three tests, AND the Census defect that decides it.
#
# With the trading day ACCEPTED, estend=F, so :463's `IF(estend)` skips the
# no-user fit and `aicnus` is used UNINITIALIZED at :481 and :557. The arm that
# would have seeded it is :245's `ELSE IF(Xeastr)`, unreachable because :243
# already tested Xeastr; from the surrounding code the intent was plainly
# `ELSE IF(Xuser)`. The vendored -O2 oracle reads 0.0 out of that slot, which
# makes `aicusr + Xraicd < aicnus` true for ANY negative AICC -- the user
# regressors are accepted unconditionally on this path.
#
# Companion specs: -aictest-tduser-reject (aicnus computed at :470) and
# -aictest-easuser (aicnus seeded at :457).
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
  variables = (td)
  aictest = (td user)
  user = (u1)
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
