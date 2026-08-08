# coverage: ssx11a.f:105-106 -- the sliding-spans loop overwrites Begxot/Endxot
# with the SPAN's own window before re-identifying the irregular regression's
# automatic AO outliers, and it does so inside the Ssxotl arm, so a span that is
# not re-identifying keeps whatever the main run left there.
#
# WHAT THIS SPEC DOES AND DOES NOT GATE, because the second half is the part
# worth writing down. It gates the pairing -- slidingspans{} with an
# x11regression outlier span, which nothing else in the corpus has -- and it
# does NOT gate the per-span assignment: deleting all four of those writes
# leaves this spec, and the whole suite, byte-identical.
#
# Half of that is structural. idotlr clamps its own test range (ibgtst =
# max(..,1), iedtst = min(..,Nspobs), idotlr.f:207-212), so a main-run window
# that starts before a span and ends after it collapses onto that span exactly,
# and the write can only matter where the main window starts INSIDE a span --
# which is why `outlierspan=` is here at all, and why it starts in 1955, inside
# spans 1 through 3.
#
# The other half is measured and negative: with the windows genuinely
# different, no span identifies an AO in the restricted region either way, so
# the verdict is the same. Pushing `critical=` down to 2.0 to force one makes
# BOTH the oracle and the engine die on the design-size limit before any table
# is written, and 2.8 finds nothing new. So the assignment stands on
# transcription (ssx11a.f:105-106), not on this gate -- and its absence would
# be a divergence waiting for a spec with a span-local outlier.
#
# Hand-authored; NOT produced by gen*.py.
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
  save = (d10 d11 d12 d13)
  savelog = all
}
x11regression{
  variables = (td)
  outlierspan = (1955.1, )
  critical = 3.0
  print = all
  save = (xrm b16 c16)
}
slidingspans{
  print = all
  save = (sfs chs)
}
