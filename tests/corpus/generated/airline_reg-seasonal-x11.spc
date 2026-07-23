# regression{ variables=(seasonal) }: regARIMA fixed seasonal regressors combined
# with the X-11 seasonal (x11pt3.f:272-281 Adjsea -> Facsea folded into D10). The
# SA series (D11) uses the X-11-only seasonal; D10 shows the total seasonal.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
  save   = (b1)
}
transform{ function = log }
regression{ variables = (seasonal) }
arima{ model = (0 1 1)(0 0 0) }
forecast{ maxlead = 0 }
estimate{ }
x11{ save = (d10 d11 d12 d13) }
