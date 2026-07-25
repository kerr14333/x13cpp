# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: x11pt3.f:815-822 -- with force{} on (Iyrt>0) the sliding-spans SA
# store takes the FORCED series Stci2, not Stci, and the oracle RETURNs right
# after the ssrit call. The Iyrt==0 counterpart (x11pt3.f:678-680) was ported;
# this one was not, so every force+slidingspans run stored no SA span at all.
# The chs (month-to-month SA change) spans are what measure it, and the ads
# (SA-series) spans exist at all only because force{} is on (ssap.f:209-210).
# The d10-d13/d16 goldens here gate a second thing: run_x11 must restore the MAIN
# run's /x11srs/, /adxser/, /x11fac/, /x11ptr/ and Begspn after the span replays,
# which rewrite all of them in place. Without that every table this spec dumps is
# the last span's -- wrong values under dates seven years late.
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
x11{
  print = all
  save = (d10 d11 d12 d13 d16)
  savelog = all
}
force{
  type = denton
  print = all
  save = (saa ffc)
}
slidingspans{
  print = all
  save = (sfs ads chs)
}
