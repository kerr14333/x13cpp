# coverage: spectrum{peakwidth = 2} -- Peakwd=2 -- mkpeak.f's second index-table row
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
spectrum{ peakwidth = 2 }
x11{ save = (d11) }
