# coverage: metadata{} with fewer values than keys -> parse ERROR
#   (exercises gtmtdt.f's swapped-count message, ported verbatim).
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
arima{
  model = (0 1 1)(0 1 1)
}
metadata{
  keys = (source frequency units)
  values = ("test-corpus")
}
