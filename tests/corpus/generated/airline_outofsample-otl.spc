# coverage: estimate{outofsample=yes} with OUTLIER regressors -- amdfct.f's
# :92-148 strip, which airline_outofsample.spc does not reach at all.
#
# Before each re-estimation the out-of-sample arm deletes every outlier
# regressor DATED INSIDE the three-year window from the design (dlrgef) and
# subtracts its fitted contribution out of the series (daxpy into `fotl`, then
# eltfcn SUB). It has to: the shortened model span no longer contains those
# dates, so the column would be identically zero and the estimate singular.
#
# The three regressors are chosen to cover both sides of that test on airline
# (1949.01-1960.12, so the window is 1958.01-1960.12):
#   ao1959.jan  -- inside, stripped
#   ls1960.mar  -- inside, stripped (and a DIFFERENT type, so the loop cannot
#                  be right by only handling AO)
#   ao1951.may  -- OUTSIDE, kept: the negative control for `begotl > nobsot`.
#
# Measured oracle on-vs-off: aape.0 5.8108 -> 5.9071, .2 6.9194 -> 7.2820.
#
# No x11{}: this spec exists for an ESTIMATION-phase diagnostic, and the three
# outlier regressors put its D9A ratios ~4e-10 off the golden -- the ordinary
# outlier-estimation floor, measured IDENTICAL on the within-sample twin, i.e.
# a property of the spec and not of this feature. test_d8b_d9a is byte-exact by
# design, so the spec simply does not carry the adjustment that would drag it
# in. airline_outofsample.spc covers the X-11 side.
#
# Hand-authored; NOT produced by genspecs.py.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{ function=log }
regression{ variables = (ao1959.jan ls1960.mar ao1951.may) }
arima{ model = (0 1 1)(0 1 1) }
estimate{ outofsample = yes }
