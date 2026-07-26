# coverage: history{fixmdl=yes} ALONGSIDE x11regression{} -- where the flag is
# INERT, and faithfully so. revdrv.f:309-350's Ixreg block ends with
# `CALL loadxr(T); IF(Lmodel)CALL restor(Lmodel,F,F)`, which reinstates
# Arimaf/Regfx/Iregfx FROM the ssprep snapshot before the span loop starts --
# and Revfix (revdrv.f:250-262) only ever set the LIVE copy, the re-snapshot at
# revdrv.f:380 being commented out in the Fortran. So with an x11regression{}
# present, fixmdl does nothing.
# Measured: the oracle's fixmdl=yes and default runs are BYTE-IDENTICAL here,
# while without x11regression{} the same flag moves sae 3.23e-3
# (airline_history-fixmdl). This port has to suppress its ssprep mirror -- the
# thing that makes the fix survive restor_span at all -- to reproduce that.
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
  fixmdl = yes
  print = all
  save = (sar sae chr che trr tre tcr tce)
  savelog = all
}
