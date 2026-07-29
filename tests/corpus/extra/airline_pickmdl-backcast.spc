# coverage: pickmdl{} + forecast{maxback=} -- automx.f:903-928's BACKCAST
# acceptance pass and amdfct.f's `Bckcst` arm behind it. The combination used to
# FATAL: the oracle runs it fine and the engine refused the spec outright.
#
# The Bckcst arm is the out-of-sample arm's mirror image. The design is time
# REVERSED (revrse) so the same forward machinery extrapolates backwards, the
# outlier window is the FIRST three years rather than the last (and every type is
# judged by its start, with no ramp special case), and the `ave` scale block
# reads the first three years too.
#
# The backcast mape has NO savelog key -- prtamd prints it and nothing else --
# so tests/parity/test_backcast_aape.py compares the harness's `bcstaape.*`
# against the printed block in this golden's .out. Measured there:
#   Last year 8.00, Last-1 7.32, Last-2 4.57, Last three years 6.63.
#
# See CB-33 for why `bcstlim=` cannot actually reject anything.
#
# Hand-authored; NOT produced by genextra.py, which would delete it.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
}
pickmdl{
  file = "pickmdl.mdl"
  method = best
}
estimate{
  savelog = all
}
forecast{
  maxlead = 12
  maxback = 12
}
x11{
  save = (d10 d11 d12 d13)
  savelog = all
}
