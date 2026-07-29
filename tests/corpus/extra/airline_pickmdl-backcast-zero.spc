# coverage: the BACKCAST aape on a series that crosses zero -- amdfct.f's
# `ivalue==1` branch, and the only spec in the corpus that reaches it.
#
# When the ORIGINAL-scale series dips to or below zero inside the window a
# percentage error is meaningless, so amdfct switches to an absolute error scaled
# by `ave` -- and `ave` is SEEDED TO ONE and then accumulated into, making the
# divisor (1 + sum|x|)/n rather than the mean. That Census defect had no gate at
# all before this spec, forward or backward.
#
# It also discriminates something no strictly-positive series can: which window
# the scale is taken from. `airline_zero.dat` is airline shifted down by 150, so
# its FIRST three years are negative and its LAST three are positive -- the
# backcast window trips ivalue==1 and the forecast window does not. A mutation
# that reads the forecast window on the backcast path passes every other spec in
# the suite and fails this one.
#
# The screens are widened (fcstlim=100, qlim=0, overdiff=1) because on this
# series every candidate is otherwise rejected and "None of the models were
# chosen" -- with no model there is no backcast pass to test.
#
# Measured in the golden .out: Last year 78.88, Last-1 16.65, Last-2 33.93,
# Last three years 43.15.
#
# Hand-authored; NOT produced by genextra.py, which would delete it.
series{
  title = "Intl Airline Passengers, shifted below zero"
  file = "../data/airline_zero.dat"
  start = 1949.01
  period = 12
}
pickmdl{
  file = "pickmdl.mdl"
  method = best
  fcstlim = 100
  qlim = 0
  overdiff = 1
}
estimate{
  savelog = all
}
forecast{
  maxlead = 12
  maxback = 12
}
x11{
  mode = add
  save = (d10 d11 d12 d13)
  savelog = all
}
