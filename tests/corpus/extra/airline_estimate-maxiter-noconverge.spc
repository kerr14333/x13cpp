# HAND-AUTHORED (do not regenerate). An EXPLICIT model that runs out of
# iterations: rgarma stops with Armaer=PMXIER, arima.f:711's prterr reports it
# through itrerr.f, and arima.f:1216's bare abend halts the run before X-11.
# This is itrerr's `Lauto=.false.` arm -- three remedies rather than two, with
# prarma.f's ARMA start values printed under option (2). The automdl arm is
# extra/airline_automdl-user-reg-noconverge.  See docs/M5_PORT_NOTES.md 108.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
  save   = (b1)
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  maxiter = 2
}
forecast{ maxlead=0 }
x11{ save=(d10 d11 d12 d13) }
