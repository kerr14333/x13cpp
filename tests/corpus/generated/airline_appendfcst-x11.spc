# Hand-authored parity spec (NOT genspecs.py output): gates series{appendfcst=yes}
# (getsrs.f Savfct -> agr3.f:158-160). The b1 save table is extended forward by the
# 12 forecast rows (Nfcst) to Posffc; d10-d13 stay on the observed span. Fixed
# airline model on airline data (bit-exact) isolates the append behaviour.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
  appendfcst = yes
  print = all
  save = (a1 b1)
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
forecast{
  maxlead = 12
  print = all
  save = (ftr fvr fct btr bct)
}
x11{
  print = all
  save = (d10 d11 d12 d13)
}
