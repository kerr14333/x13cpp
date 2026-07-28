# coverage: spectrum{saveallfreq = yes} -- Svallf -- emit the whole grid rather than the fixed literal peak indices
#
# Found by tools/option_sweep.py: parsed and silently dropped.
# Hand-authored; NOT produced by genextra.py.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{ function = log }
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
spectrum{ saveallfreq = yes }
x11{ save = (d11) }
