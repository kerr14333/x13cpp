# EDGE CASE: span and modelspan subsetting.
#   span      = restricts the range of the series that is analysed / adjusted.
#   modelspan = restricts the (narrower) range used to estimate the regARIMA
#               model, while the full span is still adjusted.
# The full data file is 2000.01-2025.08 (308 obs); this run analyses 2005.01
# onward and estimates the model only through 2020.12.
series{
  title     = "PAYEMS with span + modelspan subsetting"
  file      = "payems.dat"
  start     = 2000.01
  period    = 12
  span      = (2005.01, )
  modelspan = (, 2020.12)
}
transform{
  function = log
}
automdl{ }
forecast{
  maxlead = 24
}
x11{ }
