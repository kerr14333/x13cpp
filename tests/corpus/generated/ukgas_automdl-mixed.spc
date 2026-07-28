# Gates automdl{mixed=no}: whether the automatic modeller may pick a model with
# both AR and MA terms in the same (regular or seasonal) part. Lmixmd is read by
# amdid's candidate filter AND by pass2's model-of-last-resort construction
# (pass2.f:251-264, which picks iqr=1/0 and iqs from it).
#
# ukgas is the categorical case: the oracle's selected model drops from 3 ARMA
# terms to 1, and 53 .udg keys move.
#
# It is also the CONTROL of the pair. `mixed=no` was already correct here
# before pass2 was ported -- mutation-tested, this spec survives disabling
# either half of pass2 -- so what it pins is amdid's candidate filter and
# pass2's model-of-last-resort construction reading Lmixmd, not the re-entry.
# nottem_automdl-mixed is the spec that needs pass2.
series{
  title  = "UK quarterly gas consumption (NSA)"
  file   = "../data/ukgas.dat"
  start  = 1960.1
  period = 4
}
transform{
  function = log
}
automdl{ mixed = no }
estimate{ }
