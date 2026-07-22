# Hand-authored parity spec (NOT genspecs.py output): exercises the automd
# accept-default-model branch (automd.f:348-357, acceptdefault=yes). payems is
# chosen deliberately -- its automatic search selects (0 1 2), so accepting the
# default airline (0 1 1)(0 1 1) instead gives a DIFFERENT model. That makes the
# gate bite: an engine that ignores acceptdefault would search to (0 1 2) and
# mismatch the oracle's airline-model tables.
series{
  title = "US Total Nonfarm Employment (PAYEMS)"
  file = "../data/payems.dat"
  start = 2000.01
  period = 12
  print = all
  save = (a1 spc mv a18 a19 b1)
}
transform{
  function = auto
  print = all
  save = (a1c a2 a2p a2t a3 a3p a4d a4p trn)
}
automdl{
  acceptdefault = yes
  print = all
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
