# Composite example — component series 1 of 2 (North region).
# Each component is seasonally adjusted on its own; the aggregate is handled
# by total.spc via the composite spec when run through composite.mta.
series{
  title  = "Sales - North region"
  file   = "region_north.dat"
  start  = 1990.01
  period = 12
  comptype = add
}
transform{
  function = log
}
automdl{ }
x11{ }
