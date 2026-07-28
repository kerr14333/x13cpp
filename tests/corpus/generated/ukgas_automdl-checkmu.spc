# Gates automdl{checkmu=no}: whether the automatic modeller tests for a mean
# term at all. Lchkmu guards the chkmu call on the default model (automd.f:255)
# and the two later ones -- in the label-40 tstmd1 arm (:625) and in the
# label-30 finalization tail's redomd branch (:756) -- so turning it off changes
# both the model and its regressor set.
#
# Measured oracle on-vs-off: 49 .udg keys move, including nreg (the Constant
# leaves the model) and every ARMA coefficient. It reads as an ordinary option
# only now that the finalization tail runs on this path; before that the engine
# returned the mean-tested model either way.
series{
  title  = "UK quarterly gas consumption (NSA)"
  file   = "../data/ukgas.dat"
  start  = 1960.1
  period = 4
}
transform{
  function = log
}
automdl{ checkmu = no }
estimate{ }
