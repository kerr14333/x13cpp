# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: editor.f:1640-1659 -- a FIXED trading-day coefficient below -1 implies a
# negative daily weight that reweighting cannot repair, so the oracle refuses the
# run outright ("No seasonal adjustment this run"). Gated through
# test_m1_parse::test_outcome_matches_oracle.
#
# Also pins CB-38. The oracle prints "less than zero w hen specifying" -- one word
# split by a space -- because editor.f:1655 is 71 characters long and breaks `when`
# across the fixed-form continuation, so the blank pad at column 72 lands inside it.
# Verified against the vendored binary, not inferred.
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
estimate{}
x11{}
x11regression{
  variables = (td)
  b = (-1.5f 0.0f 0.0f 0.0f 0.0f 0.0f)
  reweight = yes
}
