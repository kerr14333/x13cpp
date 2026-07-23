# Explicit-model aictest=(tdnolpyear) -- exercises the TD-variant candidate
# vector (editor.f:1151, Itdtst=2 -> Tdayvc=(0,2,5)).
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{ function = log }
regression{ aictest = (tdnolpyear) }
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
