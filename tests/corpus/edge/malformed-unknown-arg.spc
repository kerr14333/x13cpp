# EDGE CASE: deliberately malformed spec.
# `frobnicate` is not a valid series-spec argument. X-13 should reject this at
# parse time with a diagnostic. The parity harness uses this to confirm the C++
# port emits byte-identical ERROR output (same message, same behaviour) as the
# Fortran oracle. This spec is EXPECTED TO FAIL — that failure is the test.
series{
  title       = "Malformed spec - unknown argument"
  file        = "../data/airline.dat"
  start       = 1949.01
  period      = 12
  frobnicate  = 3
}
x11{ }
