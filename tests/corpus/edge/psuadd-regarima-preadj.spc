# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: editor.f:2509-2516, arm 1 of 4 -- pseudo-additive adjustment is
# refused when any preadjustment factor comes from the regARIMA model. An AO
# regressor sets AdjAO, which is the first disjunct of a twelve-term test.
#
# The whole block was unported: this spec returned OUTCOME: OK and adjusted,
# where the oracle writes ERROR and "No seasonal adjustment this run". Found
# while probing pseudo-additive on a composite -- the probe spec put a level
# shift on a component and the oracle refused the component outright.
#
# Gated through test_m1_parse::test_outcome_matches_oracle, which compares the
# ERROR lines against the golden .err, not merely the outcome.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
regression{
  variables = (ao1955.jan)
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
x11{
  mode = pseudoadd
  save = (d10 d11 d12 d13)
}
