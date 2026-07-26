# coverage: the SHIFT branch of getreg.f:519-541's Leap Year splice, which
# airline_regb-td-fixed does not reach.
#
# With `variables=(td)` alone the removed "Leap Year" column is the LAST one, so
# `icol <= nbvec` is false and the splice degenerates to a single write past the
# end of the list. Adding a group AFTER trading day puts Leap Year in the middle
# (column 7 of 18), so the b= list really is shifted right from there: 17 values
# for 18 columns, and the 11 seasonal coefficients have to land one slot later
# than the user wrote them. Get the shift wrong and every seasonal coefficient is
# off by one -- which is exactly what the oracle's printed table pins here.
#
# arima{model=(0 1 1)} deliberately, NOT (0 1 1)(0 1 1): the oracle rejects a
# seasonal difference alongside seasonal regression effects.
#
# Measured oracle on-vs-off: 2.2e-2 in d10, 2.5e-2 in d11/d13, 2.6e-2 in d16.
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
  variables = (td seasonal)
  b = (-0.010f, -0.005f, -0.004f, -0.002f, 0.003f, 0.006f,
       0.02f, -0.01f, 0.03f, 0.01f, 0.00f, -0.02f, 0.01f, 0.02f, -0.01f,
       0.00f, 0.01f)
}
arima{ model=(0 1 1) }
estimate{ }
x11{ save=(d10 d11 d12 d13 d16) }
