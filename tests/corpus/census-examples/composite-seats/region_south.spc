# SEATS composite gate -- component series 2 of 2 (South region).
# See region_north.spc for why the MA coefficients are fixed.
# Hand-authored; NOT produced by any generator.
series{
  title  = "Sales - South region"
  file   = "region_south.dat"
  start  = 1990.01
  period = 12
  comptype = add
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
  ma    = (0.5f, 0.3f)
}
estimate{ }
seats{
  save = (s10 s11 s12 s13)
}
