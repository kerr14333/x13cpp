# Hand-authored (NOT produced by genextra.py -- do not regenerate this directory).
#
# The Aicstk default probe. `series{type=stock}` sets Isrflw=2, and
# editor.f:1159-1165 then rewrites the aictest TD candidate vector to the STOCK
# variants -- Tdayvc=(0,3,6), Itdtst=3 -- but only when the model carries no
# trading-day regressor already (ktd==0 .and. kstd==0). That is exactly the
# combination in which editor.f:1051-1057's `Aicstk=ctoi(igrptl...)` never runs,
# because it needs an EXISTING tdstock[n] group to read the day-of-month out of.
# So the group title addtd.f:31-36 builds comes from gtinpt.f:291's default of
# 31: "Stock Trading Day[31]".
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
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
