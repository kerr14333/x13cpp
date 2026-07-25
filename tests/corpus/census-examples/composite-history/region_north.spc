# Composite INDIRECT revision-history gate -- component 1 of 2 (North region).
#
# Same synthetic data and fixed airline model as ../composite-fixed/, plus a
# history{} of the seasonally adjusted series in EVERY spec of the metafile.
# That is what turns Indrev on (gtrvst.f:361-435): the first component's
# history{estimates=(sadj) start=} sets Indrev=1 and stamps Indrvs, and each
# later component must match it or the indirect analysis is dropped. Each
# component then folds its own concurrent/final SA into the shared /revdta/
# Cncisa/Finisa (putrev.f:25-30) by its comptype/compwt, and the total prints
# the aggregate as iar/iae (revdrv.f:838-846).
#
# Hand-authored; NOT produced by gen*.py.
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
history{
  estimates = (sadj)
  start = 1996.jan
  save = (sar sae)
  savelog = all
}
