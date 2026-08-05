# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: the change-of-regime arm of ssmdl.f's group walk (:150-241), which
# is WALLED in this port -- and this spec exists to prove the wall fires where
# the oracle also refuses, because before it the engine wrote both span tables
# in full and reported OUTCOME: OK.
#
# The oracle does not survive this spec either. ssmdl.f:157-159 looks for the
# change-of-regime date with
#
#     idtpos = index(igrptl,'(before ') + 8
#     IF(idtpos.eq.8) idtpos = index(igrptl,'(change from before ') + 20
#
# but no title producer in the whole tree writes `(change from before ` --
# addlom.f:63, addtd.f:88 and adrgim.f:72/178 all write `(change FOR before `,
# and regvar.f:334, savmdl.f:346 and editor.f:1816 all SEARCH for `for`. The
# title here is `Trading Day (change for before 1955.Jan)`, so both searches
# miss, `ctodat` is handed position 20 (the space before "for"), the date parse
# fails, and the run prints its change-of-regime NOTE and then halts with
# "Program error(s) halt execution". See tools/census_bugs.md CB-39.
#
# So there is no sfs/chs golden for this spec and there is not meant to be:
# what is gated is that the engine produces no span table either. Do not
# "fix" it by blessing whatever the engine happens to emit.
#
# IT LIVES IN edge/, NOT extra/, AND THAT IS LOAD-BEARING. The table and
# diagnostic gates across tests/parity discover their cases from
# tests/golden/extra and every one of them asserts the harness exited 0 -- which
# for a spec that FATALs it cannot. Blessed into extra/ this spec produced 14
# failures with nothing wrong underneath. edge/ is where the specs whose SUBJECT
# is a refusal already live, and no table gate scans it.
#
# Those 14 did settle one thing on the way past: the oracle halts LATER than
# this port refuses. It finishes the whole X-11 spine and writes D10-D16 and its
# .udg before dying in the sliding-spans setup, so the wall is a GAP in
# docs/WALLS.md and not a faithful refusal.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
}
regression{
  variables = (td/1955.jan/)
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
  save = (d10 d11 d12 d13 d16)
}
slidingspans{
  print = all
  save = (sfs chs)
}
