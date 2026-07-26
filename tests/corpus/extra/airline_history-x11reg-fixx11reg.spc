# coverage: history{fixx11reg=yes} (Revfxx, revdrv.f:309-330) -- hold the
# x11regression{} daily weights at the main run's values for every span. Parsed
# and silently dropped before; and it could not be ported until the DEFAULT path
# was right, because the port already did what the flag asks for (see
# airline_history-x11reg).
#
# The fix goes on the x11reg STORE (Irgxfx/Regfxx), not the working model: each
# span's loadxr(F) copies it in and x11mdl.f:390-395/459-467's Iregfx>=2
# rmfix/addfix then strikes every fixed column, so the per-span OLS has nothing
# left to estimate. Nothing restores the store between spans, so unlike fixmdl/
# fixreg this needs no ssprep mirror.
# Measured oracle on-vs-off: sar 8.2e-1, sae 8.3e-3.
# Hand-authored; NOT produced by gen*.py.
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
x11{
  print = all
  savelog = all
}
x11regression{
  variables = (td)
  print = all
}
history{
  estimates = (sadj sadjchng trend trendchng)
  start = 1955.jan
  fixx11reg = yes
  print = all
  save = (sar sae chr che trr tre tcr tce)
  savelog = all
}
