# coverage: pickmdl{outofsample=yes} + forecast{maxback=} -- the OUT-OF-SAMPLE
# BACKCAST, i.e. BOTH of amdfct's optional arms at once, which is the only place
# the two interact.
#
# It is the mirror of the out-of-sample forecast arm in every direction at once:
# each pass walks the model span START forward a year instead of the end back,
# and the ACTUALs are the year about to be dropped, taken in REVERSE order and
# stashed by hand -- amdfct.f:239 skips `subset` on exactly this path, because
# the design row it would name has already left the estimation.
#
# Measured in this golden's .out (printed only, no savelog key):
#   Last year 8.19, Last-1 7.01, Last-2 4.89, Last three years 6.70.
# The selected model differs from the within-sample twin's for the same reason
# it does in airline_pickmdl-outofsample: the aape is an acceptance screen.
#
# Hand-authored; NOT produced by genextra.py, which would delete it.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
}
pickmdl{
  file = "pickmdl.mdl"
  method = best
  outofsample = yes
}
estimate{
  savelog = all
}
forecast{
  maxlead = 12
  maxback = 12
}
x11{
  save = (d10 d11 d12 d13)
  savelog = all
}
