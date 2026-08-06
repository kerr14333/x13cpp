# Hand-authored (NOT produced by genextra.py).
# u1 is zero through 1958.12 and a ramp after. With the default four spans
# (1951-1957, 1952-1958, 1953-1959, 1954-1960) spans 1 and 2 see an
# identically zero column and spans 3 and 4 see the ramp.
#
# THE fixmdl DEFAULT ARM. `slidingspans{fixmdl=yes}` (Ssinit==1) has already set
# Regfx TRUE for every column at ssmdl.f:348, and chusrg.f:45 only looks at
# columns that are NOT fixed -- so chusrg can never fix anything here and the
# sspdrv.f:250-260 NOTE is absent. Measured on the oracle: no NOTE block, and
# bakusr runs ONCE (from ssmdl) rather than per span. The sibling
# -zerospan-fixmdlno spec is the same series with the other arm.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
  save   = (b1)
}
transform{
  function = log
}
regression{
  user = (u1)
  data = (
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.000000  0.000000  0.000000  0.000000  0.000000  0.000000
     0.010000  0.020000  0.030000  0.040000  0.050000  0.060000
     0.070000  0.080000  0.090000  0.100000  0.110000  0.120000
     0.130000  0.140000  0.150000  0.160000  0.170000  0.180000
     0.190000  0.200000  0.210000  0.220000  0.230000  0.240000
     0.250000  0.260000  0.270000  0.280000  0.290000  0.300000
     0.310000  0.320000  0.330000  0.340000  0.350000  0.360000
  )
  start = 1949.01
}
arima{
  model = (0 1 1)(0 1 1)
}
x11{ save=(d10 d11 d12 d13) }
slidingspans{
  save = (sfs chs)
  print = all
}
