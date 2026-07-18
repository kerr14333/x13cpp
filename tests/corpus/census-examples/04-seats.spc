# Census X-13ARIMA-SEATS manual — SEATS (ARIMA model-based) example.
# Log transform, airline model, then SEATS seasonal adjustment.
# NOTE: seats{} and x11{} are mutually exclusive — a spec may contain one
# or the other, never both.
series{
  title  = "International Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{
  function = log
}
regression{
  variables = (td easter[8])
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
forecast{
  maxlead = 12
}
seats{ }
