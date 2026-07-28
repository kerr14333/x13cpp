# Gates automdl{ljungboxlimit=}: the acceptance limit Pcr for the Ljung-Box Q
# residual diagnostic. It has TWO consumers and the second is what kept this
# argument fatal for three sessions -- pass2.f:160-169 does not merely READ Pcr,
# it INCREMENTS it (+0.025 on the first pass, +0.015 after) and then redoes the
# whole identification when the surviving model's confidence still exceeds it.
# The other consumer is the acceptdefault test at automd.f:348.
#
# Measured oracle on-vs-off at 0.5 (default 0.95): 86 .udg keys move. The
# direction matters -- at 0.99 this series measures 0, so a probe that only
# raised the limit would have called the argument INERT.
series{
  title  = "Nottingham monthly temperature (NSA)"
  file   = "../data/nottem.dat"
  start  = 1920.01
  period = 12
}
transform{
  function = log
}
automdl{ ljungboxlimit = 0.5 }
estimate{ }
