# Composite example — component series 2 of 2 (South region).
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
automdl{ }
x11{ }
