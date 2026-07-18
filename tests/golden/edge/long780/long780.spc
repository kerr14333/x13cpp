# EDGE CASE: series at exactly the program's maximum monthly length.
# POBS = PYR1 * PSP = 65 * 12 = 780 (oracle/fortran/srslen.prm). This series
# has exactly 780 monthly observations (1950.01 - 2014.12), sitting right at the
# limit. Parity must hold at the boundary.
series{
  title  = "Synthetic monthly series at the 780-observation limit"
  file   = "long780.dat"
  start  = 1950.01
  period = 12
}
transform{
  function = auto
}
automdl{ }
x11{ }
