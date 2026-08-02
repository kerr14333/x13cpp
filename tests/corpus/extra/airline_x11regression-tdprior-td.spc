# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: x11pt1.f:235's `IF(Axrgtd)Kswv=Kswv+2`. tdprior weights (Kswv=1)
# TOGETHER WITH an x11regression trading-day model (Axrgtd) is the only route to
# Kswv==3, which changes three separate things downstream:
#   - xrgtrn.f:36 -- the irregular transform becomes Xnstar*X - Xnstar (not - Xn)
#   - x11mdl.f:786 -- the estimated TD weights are COMBINED with the priors
#     (Dx11 = Dx11 + Dwt - 1) and Faccal/Factd recomputed through x11ref (Kswv=4)
#   - mkshdr/mkspst/prtfct/tdset -- header + table selection
# The four existing tdprior specs all omit `variables=(td)`, so none reaches it.
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
  print = all
  save = (d10 d11 d12 d13 d16)
  savelog = all
}
x11regression{
  variables = (td)
  tdprior = (1.4 1.4 1.4 1.4 1.4 0.5 0.5)
  print = all
  save = (a4 xrm b16 c16)
}
