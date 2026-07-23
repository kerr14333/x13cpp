# transform{adjust=lom} prior length-of-month adjustment carried through X-11
# (x11pt3.f:556-569 prior-LOM fold-in + rmpadj). Isolates the LOM-prior-through-
# X-11 D10-D13 path (no force).
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
  save   = (b1)
}
transform{ function=log adjust=lom }
arima{ model=(0 1 1)(0 1 1) }
estimate{ }
x11{ save = (d10 d11 d12 d13) }
