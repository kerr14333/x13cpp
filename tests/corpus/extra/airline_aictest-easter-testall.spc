# Hand-authored (2026-08-10). regression{testalleaster=yes} -> Lceaic
# (getreg.f:500-506).
#
# Lceaic was PARSED AND DISCARDED, which left the arm that reads it
# (editor.f:1419 / :1430-1433, ported 2026-08-09 as part of entry 103) DEAD.
# The flag appends the 99 sentinel to Easvec, and easaic reads 99 as "also test
# the model carrying ALL the Easter columns at once" -- so this spec's AIC test
# runs FOUR candidates (none / easter[8] / easter[15] / easter[8]+easter[15])
# where airline_aictest-easter-existing runs three. Measured on the stock
# oracle: the fourth block is `Likelihood statistics for model with
# easter[8]+easter[15]`, AICC 987.9350.
#
# Lceaic is only reachable through editor.f's igrp>0 arm, i.e. with an Easter
# group ALREADY in variables= -- with the bare `aictest=(easter)` fallback the
# flag is ignored entirely. That is why this spec names both columns.
series{
  title  = "Airline"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{ function = log }
regression{
  variables = (easter[8] easter[15])
  aictest = (easter)
  testalleaster = yes
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
