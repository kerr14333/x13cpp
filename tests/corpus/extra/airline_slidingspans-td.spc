# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: the PRIADJ span-replay restore (ssprep.f:56-62 / restor.f:55) on the
# slidingspans{} driver -- the sibling of extra/airline_history-td.spc, which
# has the full explanation.
#
# Short version: x11pt2's tdlom negates Priadj after folding the length-of-month
# / leap-year prior into the model trading-day factor, and the oracle restores
# the pre-tdlom value before every span. Without that restore each span's Factd
# carries no prior and every February drifts by the leap-year factor. Both span
# drivers replay x11pt1->x11pt3, so both need the restore, and they reach it by
# different call paths -- hence a spec for each.
#
# Needs transform=log + regression{td} together: that pair is what makes the
# automatic lom/lpyear prior exist in the first place.
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
  variables = (td)
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
  save = (d10 d11 d12 d13 d16)
}
slidingspans{
  print = all
  save = (sfs chs)
}
