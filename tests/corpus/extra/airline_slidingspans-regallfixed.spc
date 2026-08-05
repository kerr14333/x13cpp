# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: ssmdl.f:57-70 -- the `Iregfx.eq.3` arm, which its obvious carrier
# does NOT reach.
#
# The obvious carrier is airline_slidingspans-regfixed: regression{variables=td}
# with every b= fixed. It lands on the Iregfx==2 arm instead, and MUTATION
# TESTING is what said so -- disabling the ==3 arm changed nothing, disabling
# the ==2 arm broke it. The reason is getreg.f's Leap Year splice (ported at
# readers_spec.cpp:1101): with `picktd` and a log transform a Leap Year column
# is inserted into the b= list, regfix.f:31 then sees a column whose value is
# DNOTST, and `allfix` never survives to promote Iregfx to 3 -- even though
# rmlnvr removes that column again a moment later.
#
# Hence `tdnolpyear`: same six trading-day contrasts, no leap-year column to
# splice, so the log transform and the multiplicative mode of the sibling specs
# are both kept and regfix.f:41 still reaches Iregfx=3.
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
  variables = (tdnolpyear)
  b = (0.001f 0.001f 0.001f 0.001f 0.001f 0.001f)
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
  fixmdl = no
  print = all
  save = (sfs chs tds ads)
}
