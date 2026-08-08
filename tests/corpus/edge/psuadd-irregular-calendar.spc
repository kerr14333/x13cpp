# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: editor.f:2517-2522, arm 2 of 4 -- pseudo-additive adjustment and
# IRREGULAR-component calendar adjustment cannot both be asked for. The trigger
# is Axrgtd (an x11regression trading-day group), which is a different flag from
# arm 1's Adjtd: the calendar effect here is estimated from the irregular, not
# from the regARIMA model, so arm 1 does not fire and this arm must.
#
# Deliberately reached with NO regression{} spec, so that arm 1's twelve-term
# test is false and this spec is provably on arm 2. A spec that reaches a
# routine is not a spec that reaches the arm.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
x11{
  mode = pseudoadd
  save = (d10 d11 d12 d13)
}
x11regression{
  variables = (td)
}
