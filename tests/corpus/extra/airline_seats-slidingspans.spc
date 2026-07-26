# NOT produced by gen*.py -- hand-authored, do not delete when regenerating.
# coverage: slidingspans{} under seats{} (x11ari.f:199 runs x11pt2 on BOTH the
# X-11 and the SEATS path; seatdg.f:101-110 stores Seatsf/Seatsa per span).
# Before this spec existed, ZERO corpus specs combined SEATS with either span
# driver and the whole family was silently dropped (OUTCOME: OK, no tables).
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
slidingspans{
  save = (sfs chs)
}
