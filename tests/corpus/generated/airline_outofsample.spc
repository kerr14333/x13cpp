# coverage: estimate{outofsample=yes} -- the OUT-OF-SAMPLE aape (amdfct.f's
# Outfct arm). The argument was parsed and then fataled: the within-sample
# computation was ported and the out-of-sample one was not, so the option was
# reachable only as a wall.
#
# What the arm does: for each of the last three years it pulls the model span
# end back another year, RE-ESTIMATES, and forecasts one year from the new span
# end -- so the forecast is made by a model that has never seen the period it is
# forecasting. Within-sample forecasts from a past origin of the model fitted to
# everything. Measured oracle on-vs-off: aape.0 5.6262 -> 5.7533, .1 2.8148 ->
# 2.9044, .2 6.3759 -> 6.7551, .3 7.6878 -> 7.6004.
#
# The other two keys that move are `nfev` 19 -> 13 and `niter` 6 -> 4, and they
# are the point of this spec as much as the aape values are: amdfct restores
# eight pieces of estimation state but NOT the optimizer counters, because its
# final rgarma (amdfct.f:299) is commented out. Every other .udg key is
# byte-identical between the two runs.
#
# Hand-authored; NOT produced by genspecs.py.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{ function=log }
arima{ model = (0 1 1)(0 1 1) }
estimate{ outofsample = yes }
x11{ save=(d10 d11 d12 d13) }
