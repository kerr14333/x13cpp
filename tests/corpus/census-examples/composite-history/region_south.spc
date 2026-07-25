# Composite INDIRECT revision-history gate -- component 2 of 2 (South region).
# See region_north.spc for what this metafile exercises.
#
# Hand-authored; NOT produced by gen*.py.
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
}
estimate{ }
x11{
  save = (d10 d11 d12 d13)
}
history{
  estimates = (sadj)
  start = 1996.jan
  save = (sar sae)
  savelog = all
}
