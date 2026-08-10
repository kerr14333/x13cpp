# Hand-authored (NOT produced by genextra.py -- do not regenerate this directory).
#
# The AUTOMDL half of the Aicstk / stock-trading-day probe. Its sibling
# `airline_aictest-td-stock.spc` runs the same test through arima.f's explicit
# path; this one runs it through automd.f, which had its own inlined copy of
# editor.f:1151-1166 hardcoded to the FLOW answer Tdayvc=(0,1,4). With
# Isrflw==2 the editor rewrites the candidate vector to the stock variants
# (0,3,6), so the oracle selects tdstock1coef[31] where this path selected
# td1coef -- a different regressor in the final model at OUTCOME: OK, with
# aictest.diff.td 18.33 against the oracle's 2.24.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
  type   = stock
}
transform{
  function = log
}
regression{
  aictest = (td)
}
automdl{ }
estimate{ }
