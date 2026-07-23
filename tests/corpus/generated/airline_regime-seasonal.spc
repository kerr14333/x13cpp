# Change-of-regime seasonal regression parity spec (regime-seasonal).
# variables=(seasonal/1955.jan/) -> a full seasonal group "(after 1955.Jan)"
# plus the regime analogs "(change for before 1955.Jan)" (adrgim.f, zeroz=0).
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{
  function = log
}
regression{
  variables = (seasonal/1955.jan/)
}
arima{
  model = (0 1 1)
}
estimate{ }
