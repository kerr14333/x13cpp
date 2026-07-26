# coverage: regression{b=} + regression{variables=(td)} under a LOG transform --
# getreg.f:519-541's LEAP YEAR SPLICE, the one part of the b= writeback that is
# not a straight copy.
#
# With Picktd set and a non-identity transform, the parsed model carries a
# "Leap Year" column that is removed later (rmlnvr) because the length-of-month /
# leap-year effect is handled by the automatic prior adjustment instead. The
# user's b= list therefore has ONE FEWER value than Nb, and getreg splices a 1.0
# in at the Leap Year column's position before the count check. Six values here
# for six TD columns; drop the log transform and the same six become an error.
#
# Measured oracle on-vs-off: 1.2e-2 in d10, 1.9e-2 in d11/d16, 3.0e-2 in d13.
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
  variables = (td)
  b = (-0.010f, -0.005f, -0.004f, -0.002f, 0.003f, 0.006f)
}
arima{ model=(0 1 1)(0 1 1) }
estimate{ }
x11{ save=(d10 d11 d12 d13 d16) }
