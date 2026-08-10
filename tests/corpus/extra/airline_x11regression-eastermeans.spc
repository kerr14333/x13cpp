# Hand-authored (2026-08-10). x11regression{eastermeans=no} -> Xelong
# (gtxreg.f:454-458).
#
# TWO defects in one spec. The option was parsed and discarded, AND the six
# x11reg.cpp sites where the oracle passes `Xelong` were passing `arima.elong`
# -- the REGRESSION spec's eastermeans, a different option over a different
# design. Both default to true (gtinpt.f:289 / :466), so the substitution was
# invisible until something set one of them.
#
# Measured on the stock oracle before the port, three specs:
#   * this spec without either option        -- the baseline
#   * + regression{eastermeans=no}           -- 0 output lines change. Elong
#     never reaches the x11regression design at all.
#   * + x11regression{eastermeans=no} (here) -- 624 output lines change.
#
# So `regression{eastermeans=no}` on a spec like this one is the discriminating
# case for the substitution, and this one is the discriminating case for the
# dropped parse. Keeping the x11regression form because it gates BOTH: the
# engine cannot produce it by reading arima.elong, which no line here sets.
series{
  title = "Airline"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{ function = log }
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
x11regression{
  variables = (td easter[8])
  eastermeans = no
}
x11{ save = (d10 d11 d12 d16) }
