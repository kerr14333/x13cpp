# Hand-authored (NOT produced by genspecs.py -- do not regenerate
# this directory).  seats{} option gate: imean-yes-seats
series{
  title = "US Exports of Goods and Services (EXPGS)"
  file = "../data/expgs.dat"
  start = 1947.1
  period = 4
  print = all
  save = (a1 spc mv a18 a19 b1)
}
transform{
  function = log
  print = all
  save = (a1c a2 a2p a2t a3 a3p a4d a4p trn)
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  print = all
  save = (itr mdl rcm est acm lks rts ref rsd rrs)
  savelog = all
}
seats{
  print = all
  save = (s12 stc s10 pss s13 psi s11 sec s14 psc s16 psa tfd sfd ofd afd yfd s18 sta wkf mdc pic
    pis pit pia gaf gac gtf gtc tac ttc faf fac ftf ftc dor dsa dtr ssm cyc ltt sse ase tse cse se2
    se3 stl)
  savelog = all
  imean = yes
}
