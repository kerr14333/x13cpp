# Hand-authored (NOT produced by genextra.py -- do not regenerate this directory).
#
# The existing-Easter-group arm of editor.f:1409-1442. When the regARIMA design
# ALREADY carries an Easter regressor, the oracle builds the aictest candidate
# list from that group -- Neasvc = endcol-begcol+2, each window read back out of
# the COLUMN TITLE ("Easter[8]" -> 8) -- and only falls back to the hardcoded
# (-1,1,8,15) when there is none. Only the fallback was ported, so this spec
# swept four windows against the oracle's two and came out with
# `aictest.e.window: -1`, i.e. dropping the Easter regressor the oracle keeps.
#
# Twin of the same defect on the x11regression side, which was found and fixed
# separately (readers_spec.cpp, editor.f:1550-1590).
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{ function = log }
regression{
  variables = (easter[8])
  aictest   = (easter)
}
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
