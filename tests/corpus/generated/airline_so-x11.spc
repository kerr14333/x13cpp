# Hand-authored parity spec (NOT genspecs.py output): a regARIMA SEASONAL-OUTLIER
# regressor (Adjso==1) driven through X-11. x11pt2 handles Facso as print-only on the
# base path (the deferred A8 accumulator, x11pt2.f:204-207); the real combine is
# x11pt3's. This gates that the x11pt2 fatal is correctly narrowed to the
# x11regression feedback path (x11pt2.f:851-859) instead of firing on every
# Adjso/Adjsea spec.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
  print = all
  save = (a1 spc mv a18 a19 b1)
}
transform{
  function = log
}
regression{
  variables = (so1955.1)
  print = all
  save = (rmx otl ao ls tc so td hol usr a10 a13)
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  print = all
  save = (itr mdl rcm est acm lks rts ref rsd rrs)
  savelog = all
}
forecast{
  maxlead = 12
  print = all
  save = (ftr fvr fct btr bct)
}
x11{
  print = all
  save = (c1 d1 e1 f1 b2 c2 d2 e2 b3 e3 c4 d4 b5 c5 d5 e5 pe5 b6 c6 d6 e6 pe6 b7 c7 d7 e7 pe7 b8 d8
    d8b e8 pe8 c9 d9 b10 c10 d10 psf fsd ars sns b11 c11 d11 sac e11 d12 tal bcf tac b13 c13 d13
    pir ira b17 c17 b20 c20 h1 chl d16 paf fad d18 e18 tad b19 c19 e4 saf trf iwf)
  savelog = all
}
