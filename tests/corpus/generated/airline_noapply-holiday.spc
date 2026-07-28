# coverage: regression{noapply=(holiday)} -- estimate the regressor but do NOT
# remove its effect from the series (getreg.f:289-317 sets Adjholiday = -1; chkadj.f
# :27-33 then reads `< 0` to clear the matching Fin* flag).
#
# The argument was in gt_regression's ARGDIC with no case in the reader, so it
# was accepted and silently dropped. Measured on the oracle, d11.f over this
# same spec: 0.42911 with no noapply, and td 0.61979 / ao 0.44078 / ls 0.52636
# / holiday 0.66599 -- four distinct values, so each group needs its own spec.
#
# Hand-authored; NOT produced by genspecs.py.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{ function = log }
regression{
  variables = (td easter[8] ao1955.jan ls1958.jul)
  noapply   = (holiday)
}
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
x11{ save = (d10 d11 d12 d13 d16) }
