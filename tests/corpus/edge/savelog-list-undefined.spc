# EDGE CASE: the LIST arm of getsvl.f (:50-115) with one good name and one bad.
# estimate{} is a spec where `all` IS legal, so this pins the lookup rather than
# the slice: `aic` resolves, `bogus` does not, and the error names the argument
# without stopping the list. Companion to savelog-all-regression.spc, which
# exercises the single-name arm (:30-46).
# EXPECTED TO FAIL.
series{
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  savelog = (aic bogus)
}
