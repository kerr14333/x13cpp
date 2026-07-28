# Second automdl{mixed=no} gate -- monthly, and the larger blast radius of the
# two (90 .udg keys move against ukgas's 53). See ukgas_automdl-mixed.spc.
#
# This is the series that identified pass2 as the missing piece: the oracle's
# final model here carries two nonseasonal AR lags while its own
# `automdl.first` line reports (1 0 0)(0 1 1), so the identification ran twice.
series{
  title  = "Nottingham monthly temperature (NSA)"
  file   = "../data/nottem.dat"
  start  = 1920.01
  period = 12
}
transform{
  function = log
}
automdl{ mixed = no }
estimate{ }
