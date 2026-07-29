# SEATS composite gate -- component series 1 of 2 (North region).
#
# Same synthetic data as ../composite-fixed/, but adjusted by SEATS instead of
# X-11. That is the whole point: gtinpt.f:1170 ANDs each component's Lx11 into
# X11agr, so ONE SEATS component sends the total's indirect adjustment down
# agr3s.f (x11ari.f:342) rather than agr3.f -- a completely different routine.
#
# The MA coefficients are FIXED rather than estimated. Measured: with them
# estimated, this port's SEATS decomposition sits 1.3e-6 from the oracle's on
# this series even though every printed coefficient agrees to 11 digits -- pure
# optimizer path noise, amplified by the canonical decomposition. The indirect
# adjustment IS the sum of the component SA series, so that lands undiluted in
# isa/isf/i18. Fixing the coefficients makes the whole metafile bit-exact
# (~5e-15), which is the same reason ../composite-fixed/ pins its model.
#
# Hand-authored; NOT produced by any generator.
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
  ma    = (0.6f, 0.4f)
}
estimate{ }
seats{
  save = (s10 s11 s12 s13)
}
