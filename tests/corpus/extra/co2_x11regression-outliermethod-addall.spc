# Hand-authored (2026-08-11). x11regression{outliermethod=} -> Ladd1x
# (gtxreg.f:377-383), parsed and DROPPED: every idotlr call on the x11reg path
# passed a hardcoded `ladd1=true`, i.e. addone, which is the default -- so the
# option agreed with the oracle for as long as nobody set it.
#
# `Ladd1x` is idotlr's `Ladd1`: addone stops each forward-addition pass after
# the single largest t-statistic, addall adds every candidate over the critical
# value at once (idotlr.f:525, 633, 729, 943).
#
# WHAT MAKES THIS SPEC RATHER THAN A SIMPLER ONE. The two arms almost always
# converge. A 154-pair oracle sweep (7 series x critical 2.8 through 4.9)
# found the forward-addition ITERATION LISTING differing every single time and
# the saved tables differing NEVER: backward deletion drops whatever addall
# over-added and both arms land on the same design. Since this port defers the
# whole iteration print, a spec of that shape would gate nothing at all.
#
# There are exactly two points in that sweep where the arms reach different
# VERDICTS, and both are of this shape: addall overruns the regression-effect
# limit. On co2 at critical=3.2 addone completes and addall halts with
# `ERROR: Adding AO1971.Sep exceeds the number of regression effects allowed`;
# payems at 4.6 is the other. So what is gated here is OUTCOME: FATAL plus the
# message, against the addone sibling's completed run -- and mutating the
# reader back to the hardcoded true turns this run into OUTCOME: OK.
#
# NOT produced by gen*.py.
series{
  title = "Mauna Loa CO2"
  file = "../data/co2.dat"
  start = 1959.01
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
x11regression{
  variables = (td)
  critical = 3.2
  outliermethod = addall
  print = all
  save = (xrm b16 c16)
}
x11{
  print = all
  save = (d10 d11 d12 d13)
  savelog = all
}
