# coverage: regression{b=} with INITIAL values only (no `f` suffix), i.e.
# regfix.f's Iregfx==1. This is the negative control for the two specs beside
# it: the values seed the optimizer but nothing is held, so on a spec that
# converges the result must be IDENTICAL to the same spec with no b= at all --
# measured on the oracle at exactly 0.000e+00 across d10-d13/d16.
#
# It is here to pin that the rmfix/addfix strip is gated on Iregfx>=2 and not on
# "b= was given". Getting that wrong would strike columns that should have been
# estimated, and the mistake would be invisible on the fixed specs.
# Hand-authored; NOT produced by genspecs.py.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
  save   = (b1)
}
transform{ function=log }
regression{
  variables = (ao1955.jan ls1960.jul)
  b = (0.05, -0.04)
}
arima{ model=(0 1 1)(0 1 1) }
estimate{ }
x11{ save=(d10 d11 d12 d13 d16) }
