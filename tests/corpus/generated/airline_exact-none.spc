# testable-now feature parity spec.
series{
  title = "Intl Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{ function=log }
arima{ model=(2 1 0)(0 1 1) }
estimate{ exact=none }
