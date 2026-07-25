# Composite INDIRECT revision-history gate -- the NEGATIVE case, component 2.
#
# This component's history starts a year LATER than the north component's, which
# is exactly what gtrvst.f:402-412 rejects: "Starting date of revisons history
# analysis must be the same for all components of a composite adjustment".
# Indrev goes to 0, the oracle writes `historyindsa: no` and emits NO iar/iae --
# while every component still produces its own sar/sae normally. This is the half
# of Indrev that a positive-only corpus cannot reach.
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
  start = 1997.jan
  save = (sar sae)
  savelog = all
}
