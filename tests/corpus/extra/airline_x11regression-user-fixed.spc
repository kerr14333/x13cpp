# Hand-authored (NOT produced by genextra.py).
# PROBE for the EDITOR's user-regressor backup, x11regression half
# (editor.f:1541-1544's bakusr(rind=1)), and the spec that finally GATES CB-40.
#
# What makes it reach addusr: x11regression{b=} fixes the user column, so
# gtxreg.f:864 sets Userfx and gtxreg's regfix() leaves Iregfx=2. x11mdl.f:391
# then strikes the fixed columns (rmfix -> dlusrg) and x11mdl.f:460 puts them
# back (addfix -> addusr(1)), and the regvar + xrm punch that follow read what
# addusr restored. No span driver is involved; the earlier -x11reg-usertype
# spec cannot reach any of this, because with no fixings Userfx is false.
#
# CB-40 is the OBSERVABLE here, not just a transcription: the oracle's own xrm
# carries seven columns with `u1` identically ZERO, because bakusr.f:50/52
# displace the SOURCE of the Userx2/Usrty2 copies and slot 1 is never written.
# Writing slot 1 correctly puts the real u1 data back in that column, so the
# mutation is visible in this spec's golden -- which the five slidingspans
# specs could not do (nothing downstream of addfix re-read the design there).
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
  usertype = (user)
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
  b = (0.0 0.0 0.0 0.0 0.0 0.0 -0.5f)
  print = all
  save = (xrm b16 c16)
}
