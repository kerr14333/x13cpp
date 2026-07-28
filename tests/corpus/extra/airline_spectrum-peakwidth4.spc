# coverage: spectrum{peakwidth = 4} -- Peakwd=4 -- mkpeak.f's fourth row, the one that also writes a stray Tup(3)
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
spectrum{ peakwidth = 4 }
x11{ save = (d11) }
