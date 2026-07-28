# Baseline automdl on a BLS CES series -- see tests/corpus/data/ces_PROVENANCE.md.
# These series exist because they are the only ones in the corpus that are both
# strongly seasonal (qsori ~615 against airline's 168) and near-tied in automdl's
# best-five list, which is what makes an automdl threshold argument observable.
# This spec pins the baseline model selection the option probes are measured
# against.
series{
  title  = "Leisure and hospitality (CES, NSA)"
  file   = "../data/ces_leis.dat"
  start  = 1990.01
  period = 12
}
transform{
  function = log
}
automdl{ }
estimate{ }
