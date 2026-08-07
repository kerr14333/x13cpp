# EDGE CASE: a REAL table name from the WRONG spec. `chs` is slidingspans{}'s
# change-of-adjustment table (LSPSSP=267); x11{}'s slice is LSPX11=118..207 and
# does not contain it, so the oracle refuses. This is the spec shape entry 91
# had to learn to write: a nonsense name only proves the lookup runs at all,
# while a real name from a neighbouring slice proves the DISPLACEMENT is right.
# EXPECTED TO FAIL.
series{
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
x11{
  save = chs
}
