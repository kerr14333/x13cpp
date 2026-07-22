# EDGE CASE: transform{ function=log } on a series with a zero (obs 13), driven
# through regARIMA (no x11, so the X-11 multiplicative-data guard does not
# pre-empt). trnfcn.f:82-87 rejects log of a non-positive value with a TWO-CHANNEL
# message: STDERR "log of zero", Mt2 (.err) "log of a zero". Gates the trnfcn.cpp
# channel-exact fix. EXPECTED TO FAIL.
series{
  file   = "airline-zero.dat"
  start  = 1949.01
  period = 12
}
transform{ function = log }
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
