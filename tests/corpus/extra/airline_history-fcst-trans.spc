# coverage: history{estimates=(fcst)} with NO fstep= (revchk.f:449-457's default
# lag list -- Nfctlg=2, leads 1 and Ny) and transformfcst=yes (Rvtrfc: the
# forecast errors are differenced on the LOG scale, prfcrv.f:88-101).
# Hand-authored; NOT produced by genextra.py.
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
  print = all
  savelog = all
}
forecast{
  maxlead = 12
}
x11{
  print = all
  savelog = all
}
history{
  estimates = (fcst)
  transformfcst = yes
  start = 1955.jan
  print = all
  save = (fce fch)
  savelog = all
}
