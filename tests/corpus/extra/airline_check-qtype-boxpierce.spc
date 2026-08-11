# Hand-authored (2026-08-11). check{qtype=} -- the argument that discriminates
# a one-character slip in a transcribed DATA statement.
#
# getchk.f:48 is `DATA qptr/1,9,11,20,22/` over QDIC='ljungboxlbboxpiercebp'.
# This port had read {1,9,11,21,23}, which slices the last two dictionary
# entries as "boxpierceb" and "p" -- so the oracle accepts `qtype=boxpierce`
# and `qtype=bp` and this reader FATALed on both with
# `Argument name "boxpierce" not found`. No corpus spec had ever set qtype,
# which is exactly why an off-by-one in a pointer table survived: the first two
# entries (ljungbox, lb) are before the slip and match either way.
#
# The observable is `iqtype: boxpierce` in the savelog, plus OUTCOME itself.
#
# NOT produced by gen*.py.
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
  print = all
  savelog = all
}
check{
  qtype = boxpierce
  print = all
  savelog = all
}
x11{
  save = (d10 d11 d12 d13)
  savelog = all
}
