# Hand-authored (2026-08-10). regression{trendtc=yes} -> Lttc (getreg.f:508-516).
#
# Lttc was PARSED AND DISCARDED by this port: every x11pt3/x11pt4 call site
# passed a hardcoded false. With Adjtc==1 (the gtinpt default) and a TC outlier
# in the model, x11pt3.f:927-931 folds the temporary change back into the FINAL
# TREND instead of leaving it in the irregular, and x11pt4.f:242 then takes the
# Stc2 branch for the E7 change table. Measured against the stock oracle before
# the port: E7 Mar-1958 moves 0.0 -> -3.6, D12 1958 total 4605 -> 4560.
#
# The tc1958.mar outlier is the same one generated/airline_out-tc1958-mar.spc
# uses; this spec adds x11{} so the fold has somewhere to land.
series{
  title = "Airline"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{ function = log }
regression{
  variables = (tc1958.mar)
  trendtc = yes
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
x11{ save = (d10 d11 d12 d13 d16 e7) }
