# Composite gate — component series 1 of 2 (North region).
#
# Same synthetic data as ../composite/, but with a FIXED airline model instead of
# automdl{}. That is the whole point of this second copy: the INDIRECT adjustment
# is literally the sum of the component seasonally adjusted series, so any
# component estimation drift lands undiluted in the indirect tables. A fixed model
# makes every table of the metafile bit-exact gateable. ../composite/ keeps
# automdl{} as the illustrative Census-style example (and an automdl gate case).
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
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
x11{
  save = (d10 d11 d12 d13)
}
