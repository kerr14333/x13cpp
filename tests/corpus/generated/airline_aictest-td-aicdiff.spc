# Explicit-model AIC regressor test with regression{aicdiff=} -- arima.f:569
# path. aicdiff was PARSED AND DROPPED (getreg.f:405-428 had no branch in
# gt_regression), so every AIC test used the gtinpt default threshold of 0.0
# whatever the spec asked for. The no-TD-vs-best-TD AICC gap here is 18.33, so
# aicdiff=(20.0) makes the oracle REJECT trading day where the default accepts
# it -- nreg 0 against 1, and every coefficient follows.
# NB generated/cover_reg-aicdiff already carried `aicdiff=0.0`, which is the
# DEFAULT and therefore inert: it could never have caught this.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{
  function = log
}
regression{
  aictest = (td)
  aicdiff = (20.0)
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
