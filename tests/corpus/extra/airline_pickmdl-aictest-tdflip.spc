# Hand-authored (NOT produced by genextra.py -- do not expect a
# regeneration to recreate it).
# coverage: automx.f:259-296's Picktd RESTORE -- the pickmdl branch taken when
#           the winning candidate's trading-day AIC verdict DIFFERS from the
#           last candidate estimated. Reaching it needs `aicdiff` tuned between
#           two candidates' AICC gaps: they are 18.33 18.82 18.85 18.49 20.20 on
#           this series, so 19.0 accepts trading day for candidate 5 and rejects
#           it for the winner (candidate 2). The observable is d11/d13 on
#           FEBRUARIES: the restore returns Adj to all-1 while Sprior must keep
#           the prior tdaic.f:600-623 wrote, and getting that wrong is worth
#           exactly (days-in-Feb / 28.25) -- 0.885% low non-leap, 2.655% high
#           leap. d10/d12/d16 are unaffected either way, so they do NOT gate it.
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
  aictest = (td)
  aicdiff = 19.0
  print = all
}
pickmdl{
  mode = fcst
  file = "pickmdl.mdl"
  method = best
  identify = all
  print = all
}
estimate{
  print = all
  savelog = all
}
forecast{
  maxlead = 12
  print = all
}
x11{
  print = all
  save = (d10 d11 d12 d13 d16)
  savelog = all
}
