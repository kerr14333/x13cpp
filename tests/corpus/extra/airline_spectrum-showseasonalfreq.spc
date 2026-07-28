# coverage: spectrum{showseasonalfreq = yes} -- Lprsfq -- plot the seasonal grid with no trading-day splice
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
spectrum{ showseasonalfreq = yes }
x11{ save = (d11) }
