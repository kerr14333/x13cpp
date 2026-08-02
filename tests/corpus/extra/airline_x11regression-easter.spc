# Hand-authored (NOT produced by genextra.py).
# coverage: editor.f:1727-1747 -- the choice of EXTREME-VALUE METHOD for the
# irregular regression, made once at spec-read from the parsed x11reg model.
#
# An explicit `easter[8]` REGRESSOR (not aictest=(easter)) puts a holiday group
# in the model, and Holgrp>0 disqualifies the 2.5-sigma tdxtrm clip: the oracle
# takes the other arm and runs automatic AO outlier identification instead,
# picking up seven AO regressors here. This port used to test only Xeastr --
# the AIC-TEST flag -- so it read Sigxrg=2.5 and fitted an 8-column design
# against the oracle's 15. Wrong numbers at OUTCOME: OK; c16 was 100% out.
#
# Companion: -aictest-user2 reaches the same arm through CB-36's stale rtype.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{ function = log }
arima{ model = (0 1 1)(0 1 1) }
estimate{ print = all savelog = all }
x11{ print = all save = (d10 d11 d12 d13 d16) savelog = all }
x11regression{
  variables = (td easter[8])
  print = all
  save = (xrm b16 c16)
}
