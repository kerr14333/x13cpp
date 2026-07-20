# testable-now feature parity spec.
series{
  title = "Airline (span subset)"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
  span = (1952.01, 1960.12)
}
transform{ function=log }
arima{ model=(0 1 1)(0 1 1) }
estimate{ }
