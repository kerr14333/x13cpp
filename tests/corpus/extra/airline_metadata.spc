# coverage: metadata{} keys/values roundtrip (valid, equal counts)
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  print = all
  savelog = all
}
x11{
  print = all
  savelog = all
}
metadata{
  keys = (source frequency units)
  values = ("test-corpus" "monthly" "passengers")
}
