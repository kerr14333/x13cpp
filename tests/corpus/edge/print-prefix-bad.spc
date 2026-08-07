# EDGE CASE: getprt.f's +/- prefix arm, reached whenever the token is not a
# NAME (gtdcnm returns argok=false). An integer is not "+" or "-", so both the
# single-value arm (getprt.f:66-68) and the list arm (getprt.f:147-148) refuse
# it -- and the two messages DIFFER BY ONE CHARACTER: the single-value one ends
# `or nothing.` and the list one ends `or nothing` with no period. Census
# inconsistency, reproduced verbatim; this spec is what pins it.
# EXPECTED TO FAIL (both messages).
series{
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  print = 7
}
x11{
  print = (12 d11)
}
