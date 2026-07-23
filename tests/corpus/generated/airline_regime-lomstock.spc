# Change-of-regime stock length-of-month regression (variables=(lomstock/1955.jan/)).
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
  variables = (lomstock/1955.jan/)
}
arima{
  model = (0 1 1)
}
estimate{ }
