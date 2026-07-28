# Second automdl{checkmu=no} gate -- monthly, against ukgas's quarterly.
# Measured oracle on-vs-off: 35 .udg keys move.
#
# The log transform is load-bearing and is NOT the natural choice for a
# temperature series: under `function = none` the same argument moves only 2
# keys and the spec would pin almost nothing. Same trap as the probe-value
# lesson in tools/automdl_scouting.md -- a spec can exercise an option's
# plumbing while measuring none of its effect.
series{
  title  = "Nottingham monthly temperature (NSA)"
  file   = "../data/nottem.dat"
  start  = 1920.01
  period = 12
}
transform{
  function = log
}
automdl{ checkmu = no }
estimate{ }
