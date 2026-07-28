# Gates automdl{maxorder=}: the upper bounds on the ARMA orders amdid may
# search, against the (2 1) default. Maxord(2) is additionally read by pass2's
# model-of-last-resort (pass2.f:258-264), which is why this argument and
# `mixed` failed together and closed together.
#
# ukgas is the one series in the probe set where it was still wrong after the
# path unification: 69 .udg keys move, the selected model drops from 3 ARMA
# terms to 2, and the engine's nefobs was 104 against the oracle's 103 -- a
# DIFFERENCING order the first identification pass did not choose, i.e. the
# oracle had re-identified through pass2. On nottem/ces_accfood/ces_leis the
# same value was already applied, which is why the two remaining arguments had
# to be re-measured separately rather than assumed to share a cause.
series{
  title  = "UK quarterly gas consumption (NSA)"
  file   = "../data/ukgas.dat"
  start  = 1960.1
  period = 4
}
transform{
  function = log
}
automdl{ maxorder = (1 1) }
estimate{ }
