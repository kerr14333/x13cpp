# Composite force gate — component series 2 of 2 (South region).
#
# DELIBERATELY carries no force{}, unlike the north component. That
# asymmetry is what makes ../composite-force-indno gate anything: with
# both components forced the same way, benchmarking commutes with the
# sum and the oracle's iaa is bit-identical for indforce=yes and
# indforce=no (measured 4.9e-15 apart, for denton and regress alike),
# so no gate can tell the two arms of agr3.f:436 apart. With only one
# component forced they separate by 1.4e-05.
#
# Same synthetic data as ../composite/, but with a FIXED airline model instead of
# automdl{}. That is the whole point of this second copy: the INDIRECT adjustment
# is literally the sum of the component seasonally adjusted series, so any
# component estimation drift lands undiluted in the indirect tables. A fixed model
# makes every table of the metafile bit-exact gateable. ../composite/ keeps
# automdl{} as the illustrative Census-style example (and an automdl gate case).
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
