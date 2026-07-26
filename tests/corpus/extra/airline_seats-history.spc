# NOT produced by gen*.py -- hand-authored, do not delete when regenerating.
# coverage: history{} under seats{} (revdrv.f:5 takes Lseats -> x11ari; the
# per-span SEATS components reach the SAME getrev x11pt3 feeds Sts/Stci/Stc,
# seatdg.f:148-181). Same silent-drop class as the slidingspans spec beside it.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  savelog = all
}
seats{
  save = (s10 s11 s12 s13)
  savelog = all
}
history{
  start = 1955.01
  estimates = (sadj trend)
  save = (sar sae trr tre)
  savelog = all
}
