# Census X-13ARIMA-SEATS manual — "Getting Started" example.
# Basic X-11 seasonal adjustment of the airline series, all defaults.
#
# Run from this directory so the relative data path resolves:
#   x13as -i 01-basic-x11 -o 01-basic-x11.out
series{
  title  = "International Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
x11{ }
