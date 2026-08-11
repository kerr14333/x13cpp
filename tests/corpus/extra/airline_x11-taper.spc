# Hand-authored (2026-08-11). x11{taper=} -> Thtapr (getx11.f:479-493), the
# Tukey-Hanning taper sautco applies to the series before the autocovariances
# (sautco.f:18, `IF(R.gt.0D0)CALL taper`). It was parsed and DROPPED, and the
# port's sautco had no taper at all -- invisible because gtinpt.f:336 defaults
# Thtapr to 0 and no corpus spec ever set it.
#
# Measured on the stock oracle before porting: adding `taper = 0.3` to a run
# with these spectrum tables moves 484 lines of output. The taper is applied
# ONLY on the arspec path -- spcdrv.f hands Thtapr to spgrh and not to spgrh2 --
# so `type = arspec` is load-bearing here, not decoration.
#
# NOT produced by gen*.py.
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
  taper = 0.3
  print = all
  savelog = all
  save = (d10 d11 d12 d13)
}
spectrum{
  type = arspec
  maxar = 30
  qcheck = yes
  print = all
  save = (sp0 sp1 sp2 spr st0 st1 st2)
  savelog = all
}
