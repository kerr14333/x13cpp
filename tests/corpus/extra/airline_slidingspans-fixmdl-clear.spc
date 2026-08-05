# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: slidingspans{fixmdl=CLEAR}, the third value of the option and the
# only route to Ssinit==2 -- INTDIC is 'no','yes','clear' and Ssinit is
# ivec(1)-1 (getssp.f:50/156), so no/yes give 0/1 and nothing else reaches 2.
#
# What it gates is sspdrv.f:130-143, "reset model parameters to original
# values": Arimap for every non-fixed ARMA parameter, B when Iregfx==0, Bx when
# Irgxfx==0, all set to DNOTST so each span re-estimates from cold starting
# values instead of from its predecessor's converged ones.
#
# PLACEMENT is the whole point of the spec. That block sits between ssx11a
# (whose tail is `CALL restor`, which copies Ap2 back over Arimap) and x11ari
# at sspdrv.f:180 (which runs arima and therefore the estimation). Read as
# "after the span" it is dead code -- the next restor overwrites it -- and read
# as "before the span's estimation" it is live. Only the call order settles it,
# and this spec is what makes the answer observable.
#
# Identical to airline_slidingspans-fixmdl-clear's sibling
# airline_slidingspans-fixmdl-no except for the one word, so the pair also
# measures what `clear` costs relative to plain re-estimation.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
}
regression{
  variables = (td)
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  savelog = all
}
x11{
  savelog = all
  save = (d10 d11 d12 d13 d16)
}
slidingspans{
  fixmdl = clear
  print = all
  save = (sfs chs)
}
