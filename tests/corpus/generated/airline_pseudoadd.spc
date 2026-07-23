# x11{mode=pseudoadd}: pseudo-additive decomposition (Psuadd). Exercises the
# x11pt3 pseudo-additive D10/D11 branch (x11pt3.f:245-249, Stci=Stcsi-Stc*(Sts-1))
# on the no-model direct-X11 path.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
  save   = (b1)
}
x11{ mode = pseudoadd  save = (d10 d11 d12 d13) }
