# NOT produced by gen*.py -- hand-authored, do not delete when regenerating.
# coverage: editor.f:1577-1591's Easter AIC-test WINDOW SET, the `Easgrp>0` arm.
#
# When `variables=` names Easter regressors, the windows the AIC test sweeps are
# THEIRS -- Neasvx = endcol-begcol+2, and Xeasvc(2..) is read back out of the
# column titles ("Easter[8]" -> 8, editor.f:1556-1561). Only when no Easter
# group is in the model does the default {0,1,8,15} sweep apply.
#
# This engine used to take the default branch unconditionally, because it was
# written in the `aictest=` PARSER (gt_x11regression argidx 19) where the
# question cannot be answered: `variables=` may not have been read yet, so
# Easgrp is unknown. The oracle decides it in the editor, once the whole spec is
# in, and so does xrg_editor_setup now. Before the fix this spec reported four
# AICCs (noeaster/easter01/easter08/easter15) where the oracle reports two.
#
# `td` is in variables= on purpose and is what makes the spec GATEABLE rather
# than walled. x11aic.f:112-143's strip loop deletes the Easter columns whenever
# Xeastr is on, so an Easter-only variables= list arrives at the i==1 baseline
# with an empty design -- and `xeastr` also suppresses editor.f:1727's
# Sigxrg=2.5 default, putting the run on the automatic-AO branch, where the port
# and the oracle still disagree (~7.6 in AICC; walled, see readers_spec.cpp).
# A surviving TD group avoids that arm entirely and both agree bit-exact.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  print = all
  savelog = all
}
x11{
  print = all
  savelog = all
}
x11regression{
  variables = (td easter[8])
  aictest = (easter)
  print = all
  save = (xrm b16 c16)
}
