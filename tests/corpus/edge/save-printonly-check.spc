# EDGE CASE: PRINT and SAVE are different dictionaries over the same table
# slots. check{}'s `acfplot`/`acp` is printable but not savable, and stable.prm
# encodes that by leaving its two entries EMPTY -- an empty dictionary entry can
# never match a NAME token, so the emptiness is the refusal. Nothing else in the
# corpus distinguishes table.prm from stable.prm; without this spec, a port that
# used the PRINT dictionary for both would gate green.
# EXPECTED TO FAIL.
series{
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
check{
  print = acfplot
  save  = acfplot
}
