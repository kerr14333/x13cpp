# Gates automdl{cancel=}: the AR/MA near-cancellation tolerance (Cancel, read by
# tstmd2 / the finalization tail's insignificant-coefficient drop). Two roots
# closer together than this are treated as cancelling and the pair is removed.
#
# The DIRECTION matters and is counter-intuitive: 0.05 is BELOW the 0.1 default,
# i.e. a TIGHTER tolerance, and that is the value that bites here (32 .udg keys
# move). Raising it instead -- 0.3, 0.5, 0.9 -- measures 0 on this series, so a
# probe that only pushed the value up would have reported the argument INERT.
# See tools/automdl_scouting.md: the sweep prints the same word for "bad probe"
# and for "does nothing".
#
# As with nottem_automdl-checkmu, `function = log` is what makes the option
# observable at all on this series.
series{
  title  = "Nottingham monthly temperature (NSA)"
  file   = "../data/nottem.dat"
  start  = 1920.01
  period = 12
}
transform{
  function = log
}
automdl{ cancel = 0.05 }
estimate{ }
