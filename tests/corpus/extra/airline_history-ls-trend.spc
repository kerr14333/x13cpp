# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: x11pt3.f:1067-1072 -- the revisions-history TREND store picks its
# buffer, and the arm it picks on is `(.not.Finls).and.Adjls.eq.1`: the PUBLISHED
# D12, with the level shift folded back in (stc2), rather than the internal Stc.
# Before getrev was ported into x11pt3 this port re-read ctx.x11srs.stc after the
# pass, which is the LS-FREE trend, so every trend revision on a spec with a
# level shift was taken from the wrong series. Invisible on every earlier history
# spec because with no LS regressor Facls is the identity and the two agree.
#
# Note the buffer is NOT `have_stc2`: stc2 also exists when only the TC fold or
# transform{temppriortrend=} built it, and on those the oracle hands getrev the
# internal Stc anyway. This spec is on the LS arm; that disagreement is
# transcribed at the call site and has no carrier.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
}
regression{
  variables = (ls1955.jan)
  print = all
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
history{
  estimates = (sadj sadjchng trend trendchng)
  start = 1956.jan
  print = all
  save = (sar sae chr che trr tre tcr tce)
  savelog = all
}
