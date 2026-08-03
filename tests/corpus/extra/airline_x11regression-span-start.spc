# Hand-authored (NOT produced by genextra.py).
# coverage: x11mdl.f:113-118 -- `x11regression{span=}` narrowing the irregular
# regression's START, and the :512-528 setspn restore + regvar rebuild that
# puts the span back before the FACTOR is built (fit narrow, apply wide).
#
# Two traps this spec is the only thing standing on:
#  - tdset. The oracle calls it ONCE, from editor.f:2240, over the whole
#    [Pos1bk,Posffc] buffer; this port issues it inside x11mdl_td, so it has to
#    be fed the span start the BUFFER is indexed from, not the narrowed one.
#    Feeding it the narrowed date slides Xnstar/Xn by nbeg periods and cost
#    4.6e-2 on d11 -- worse than ignoring the option altogether.
#  - the restore has to happen even on an early return, or Begspn/Nspobs leak
#    narrowed into the rest of the X-11 run.
#
# A span that ends EARLY is a different mechanism (xrgdrv.f:152-158's Xdsp) and
# is refused; see docs/WALLS.md.
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
  savelog = all
  save = (d10 d11 d12 d13)
}
x11regression{
  variables = (td)
  span = (1953.07,1960.12)
  print = all
  save = (xrm b16 c16)
}
