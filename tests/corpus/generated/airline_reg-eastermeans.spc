# coverage: regression{eastermeans=no} -- Elong, how the Easter regressor's
# long-run mean is computed (getreg.f:280-286; regvar.f:225 hands it to estrmu).
# `yes` (the default) uses the exact 1600-year distribution of the Easter date,
# `no` the calendar-month means. Parsed and silently dropped; 26 .udg keys move.
#
# Hand-authored; NOT produced by genspecs.py.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{ function = log }
regression{ variables = (easter[8]) eastermeans = no }
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
x11{ save = (d10 d11 d12 d13 d16) }
