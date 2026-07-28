# coverage: regression{tcrate=} -- the TC outlier decay rate (getreg.f:386-404
# -> Tcalfa/Havtca). Parsed and silently dropped: the run used gtinpt.f:1217's
# 0.7^(12/Sp) default instead. Measured on the oracle, 64 .udg keys move.
#
# outlier{tcrate=} is the SAME latch (gtotlr.f:229-246) and was equally silent;
# it is gated by airline_outlier-tcrate beside this. The two specs refuse to
# coexist -- "Cannot specify tcrate in both the regression and outlier specs" --
# which is why Havtca has to be a latch and not a last-one-wins.
#
# Hand-authored; NOT produced by genspecs.py.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{ function = log }
regression{ variables = (tc1955.jan) tcrate = 0.5 }
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
x11{ save = (d10 d11 d12 d13 d16) }
