# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: `series{format=}` (getsrs.f:466 -> gtfldt.f:71), which is WALLED in
# this port. Before the wall the option was PARSED AND DROPPED: series.cpp set
# havfmt and nothing read it, so a spec naming a file layout got the
# free-format read and OUTCOME: OK with whatever that produced.
#
# `free` is deliberately the value used here, because it is the arm that looks
# like a no-op and is not one. gtfldt.f:72-75 runs BEFORE the layout dispatch:
#
#     IF(Havfmt)THEN
#      xfmind=strinx(F,X12FMT,x12ptr,1,PX12F,Datfmt)
#      IF(.not.Hvfreq.and.Hvstrt.and.xfmind.ne.9)THEN
#       Freq=12
#       Hvfreq=T
#      END IF
#
# so naming ANY layout except `tramo` defaults the period to 12 when the spec
# gave a start date and no period. This spec gives `period = 12` explicitly, so
# the ORACLE reads it exactly as free format and runs to completion -- which is
# the point: the wall is a GAP, not a faithful refusal, and the golden proves
# it by being a complete successful run.
#
# IT LIVES IN edge/, NOT extra/, for the reason spelled out in
# airline_slidingspans-regime-td.spc: every table gate discovers its cases from
# tests/golden/extra and asserts the harness exited 0, which a spec whose
# SUBJECT is a refusal cannot do.
#
# Gated by tests/parity/test_m1_parse.py::test_engine_wall_refuses_with_message.
# When the layout readers are ported, that test fails LOUDLY -- delete the entry
# there and the wall in core/src/specparse/series.cpp together.
series{
  title  = "International Airline Passengers"
  file   = "../data/airline.dat"
  format = "free"
  start  = 1949.01
  period = 12
}
transform{ function = log }
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
