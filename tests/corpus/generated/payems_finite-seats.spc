# HAND-AUTHORED -- NOT produced by genspecs.py (do not expect a regenerate to
# recreate it; genspecs.py wipes *.spc in this directory, see its docstring).
# series = payems, config = finite-seats
#
# Gates seats{finite=yes} (Lfinit, the finite-sample signal-extraction
# filters).  Measured against the oracle, Lfinit never touches the
# decomposition: every table written in both modes is byte-identical.  This
# spec pins that invariance -- s10-s18 must stay bit-exact with finite=yes set.
# The flag is not inert, though: it gates getDiag (sigex.f:1502), hence ten
# extra save tables (faf/fac/ftf/ftc/gaf/gac/gtf/gtc/tac/ttc) and 44 .udg
# savelog keys, none of which the port emits yet.  See the measurement note in
# core/src/seats/seatopts.hpp and tools/census_bugs.md CB-14.
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
forecast{
  maxlead = 12
  print = all
  save = (ftr fvr fct btr bct)
}
seats{
  finite = yes
  print = all
  save = (s12 stc s10 pss s13 psi s11 sec s14 psc s16 psa tfd sfd ofd afd yfd s18 sta wkf mdc pic
    pis pit pia gaf gac gtf gtc tac ttc faf fac ftf ftc dor dsa dtr ssm cyc ltt sse ase tse cse se2
    se3 stl)
  savelog = all
}
