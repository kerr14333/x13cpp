# testable-now feature parity spec.
series{
  title = "US Exports (EXPGS)"
  file = "../data/expgs.dat"
  start = 1947.1
  period = 4
}
transform{ function=log }
arima{ model=(2 1 0) }
estimate{ exact=ma }
