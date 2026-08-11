# CB-43. `x11regression{almost=}` is a documented option that v1.1 b61 cannot
# parse: gtxreg.f:59 declares `CHARACTER ARGDIC*271` over a literal 269
# characters long, and argptr's last entry is 264..271 -- so the dictionary's
# 36th and final entry is 'almost' PLUS the two blanks Fortran pads the
# parameter with. cmpstr compares lengths exactly (cmpstr.f:11-12), so the
# token `almost` can never match it and the oracle answers
#
#   ERROR:  Argument name "almost" not found
#
# and halts. Every other entry is unaffected: only the LAST one runs into the
# padding.
#
# This port had the defect only in the sense that it did not have it. It stored
# the same literal unpadded, and std::string_view::substr CLAMPS at the end of
# the string where Fortran pads, so strinx sliced a bare "almost", matched, and
# the run returned OUTCOME: OK -- on a spec the oracle refuses. The fix is two
# trailing blanks in the literal, i.e. the DECLARED length.
#
# The value below is deliberately valid (gtxreg.f:596 wants > 0): what is being
# gated is that the ARGUMENT NAME is unreachable, not that the value is bad.
#
# Hand-authored; NOT produced by gen*.py.
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
}
x11regression{
  variables = (td)
  critical = 3.0
  almost = 1.0
}
x11{
  save = (d10 d11)
}
