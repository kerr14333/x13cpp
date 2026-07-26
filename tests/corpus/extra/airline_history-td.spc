# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: THE PRIADJ SPAN-REPLAY RESTORE (ssprep.f:56-62 / restor.f:55).
#
# This is the gate whose absence hid a real wrong-numbers bug for the whole life
# of the history{}/slidingspans{} port. x11pt2's tdlom NEGATES Priadj after
# folding the length-of-month/leap-year prior into the model trading-day factor,
# so that nothing downstream removes the prior a second time. ssprep saves the
# pre-tdlom value in Pri2 and restor puts it back before every span; this port
# did neither, so every span replay saw Priadj<=0, skipped the fold, and built a
# Factd with NO prior in it. Every FEBRUARY of the span's D11/D16 was then off by
# exactly the leap-year factor -- measured 0.9912 in a common year and 1.0265 in
# a leap year, i.e. sae 2.7e-2 against a 1e-5 floor -- all behind OUTCOME: OK.
#
# Nothing caught it because the bug needs BOTH halves at once and no corpus spec
# had them: a regARIMA trading-day regressor under a log transform (which is what
# creates the automatic lom/lpyear prior at all -- see chkadj.f/tdlom.f) AND a
# span-replay driver (history{} or slidingspans{}) to expose the missing restore.
# Either half alone is bit-exact and always was.
#
# So do not "simplify" this spec: drop transform=log, or drop the td regressor,
# or drop history{}, and it silently stops testing anything.
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
}
history{
  estimates = (sadj sadjchng seasonal trend trendchng)
  start = 1955.jan
  print = all
  save = (sar sae chr che trr tre tcr tce sfr sfe)
  savelog = all
}
