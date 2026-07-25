# coverage: x11{final=(ls)} with a level-shift regressor -- the Part-E Facls
# re-adjustment (x11pt3.f:1243-1249). With final=ls the level shift is NOT part
# of the published SA series, so the weight-zero replacement value -- which is
# built from the trend, and the trend carries the shift -- has it divided back
# out. Measured oracle on-vs-off: 6.0e-2 in e2, 3.0e-2 in d11.
# Hand-authored; NOT produced by genspecs.py.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
  save   = (b1)
}
transform{ function=log }
regression{ variables=(ls1955.jan) }
arima{ model=(0 1 1)(0 1 1) }
estimate{ }
x11{ final=(ls) save=(d10 d11 d12 d13 d16 e1 e2 e3 e11 e18) }
