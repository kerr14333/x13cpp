# coverage: outlier{tcrate=} (gtotlr.f:229-246 -> Tcalfa/Havtca), the other half
# of the shared latch. This one measured as DIFFERS rather than DROPPED in the
# sweep -- the engine was neither honouring the argument nor reproducing the
# default, because automd's own 0.7^(12/Sp) fallback filled Tcalfa at a
# different point than gtinpt.f:1217 does.
#
# Hand-authored; NOT produced by genspecs.py.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{ function = log }
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
outlier{ types = (tc) critical = 2.5 tcrate = 0.5 }
x11{ save = (d10 d11 d12 d13 d16) }
