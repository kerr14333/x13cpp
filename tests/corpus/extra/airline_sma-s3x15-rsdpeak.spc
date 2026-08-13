# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: the ORDER of the `.err` stream, which nothing else in the corpus
# pins. spcrsd.f's residual-peak WARNING is written from arima.f:1126, i.e. in
# the ESTIMATION phase, and x11fil.f's 3x15-filter WARNING comes out of x11pt2,
# after it. Every other spec that carries the residual warning carries no
# X-11-phase message at all, so both placements of the residual block -- the
# oracle's, and computing it late with the other three spectra -- produce the
# same file. This one separates them: get it wrong and the two WARNINGs come
# out swapped.
#
# Both halves are cheap and already understood. The 3x15 seasonal filter needs
# 20 years of data and airline has 12, so x11fil declines it and says so
# (`generated/cover_x11-sma-s3x15` gates that message on its own, with no
# residual peak beside it). The airline residuals carry a visually significant
# seasonal peak at s1 on essentially every spec in this corpus.
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
  savelog = all
}
x11{
  seasonalma = s3x15
  save = (d10 d11 d12)
  savelog = all
}
