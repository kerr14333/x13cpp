# Change-of-regime trading-day regression (variables=(td/1955.jan/)).
# adrgim.f TD branch: full-effect "Trading Day (after 1955.Jan)" + the regime
# analogs "(change for before 1955.Jan)", plus the Picktd leap-year analog.
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
  variables = (td/1955.jan/)
}
arima{
  model = (0 1 1)
}
estimate{ }
