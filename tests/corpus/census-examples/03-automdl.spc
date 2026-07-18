# Census X-13ARIMA-SEATS manual — automatic model identification example.
# Automatic transform selection and automatic ARIMA model identification
# (the TRAMO-style automdl procedure), then X-11 seasonal adjustment.
series{
  title  = "International Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{
  function = auto
}
regression{
  aictest = (td easter)
}
automdl{ }
forecast{
  maxlead = 24
}
x11{ }
