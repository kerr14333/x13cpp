# Hand-authored parity spec (NOT genspecs.py output): a regARIMA SEASONAL-OUTLIER
# regressor (Adjso==1) driven through X-11. Adds x11{centerseasonal=yes} (Lcentr,
# x11pt3.f:280 -> vsfc): re-centre the COMBINED seasonal with a 2xNy MA. This is
# the ONLY spec shape that makes Lcentr live -- it runs inside the Adjsea/Adjso
# block, so without a seasonal/SO regressor the oracle does not centre either and
# on/off are byte-identical. Measured oracle on-vs-off delta in d10: airline
# 2.06e-3, payems 2.95e-4, expgs 1.89e-3.
series{
  title = "US Total Nonfarm Employment (PAYEMS)"
  file = "../data/payems.dat"
  start = 2000.01
  period = 12
  print = all
  save = (a1 spc mv a18 a19 b1)
}
transform{
  function = log
}
regression{
  variables = (so2005.1)
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
  centerseasonal = yes
  print = all
  save = (c1 d1 e1 f1 b2 c2 d2 e2 b3 e3 c4 d4 b5 c5 d5 e5 pe5 b6 c6 d6 e6 pe6 b7 c7 d7 e7 pe7 b8 d8
    d8b e8 pe8 c9 d9 b10 c10 d10 psf fsd ars sns b11 c11 d11 sac e11 d12 tal bcf tac b13 c13 d13
    pir ira b17 c17 b20 c20 h1 chl d16 paf fad d18 e18 tad b19 c19 e4 saf trf iwf)
  savelog = all
}
