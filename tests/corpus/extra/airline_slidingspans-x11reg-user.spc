# Hand-authored (NOT produced by genextra.py).
# PROBE for slidingspans{} + x11regression{user=}. Body copied verbatim from
# extra/airline_x11regression-user.spc; the ONLY difference is slidingspans{}.
#
# Note what this does NOT reach: ssxmdl.f:143-148's bakusr arm is keyed on
# `Nusxrg`, which is the length of x11regression{usertype=} and NOT the user-
# column count. With no usertype= the flag is 0, so the arm is skipped and the
# span path goes through rmfix's dlusrg instead -- the x11regression user
# column is struck in span 1 and, because addusr is not called for it, never
# comes back. The sibling -usertype spec takes the other door.
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
slidingspans{
  save = (sfs chs)
  print = all
}
