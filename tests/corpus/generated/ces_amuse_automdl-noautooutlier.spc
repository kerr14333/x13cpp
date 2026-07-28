# Third automdl{noautooutlier=tramo} gate, and the CATEGORICAL one: this is the
# only series in the corpus where the argument changes the SELECTED MODEL rather
# than just its coefficients -- the oracle goes from 5 ARMA terms to 4, and 111
# .udg keys move (against 56 on ces_accfood and 20 on ukgas). That is tstmd1
# doing what it exists to do: dropping an order the identified model did not
# earn. A gate on coefficients alone could in principle pass on a
# near-equivalent model; this one cannot.
#
# ces_amuse has the narrowest best-five gap in the corpus, which is why the
# probe set EXCLUDES it (tools/automdl_scouting.md): its DEFAULT run does not
# match the oracle (engine 4 ARMA terms, oracle 5), so an option verdict
# measured there is uninterpretable. That exclusion is about the default path
# and does not extend to this spec -- with `tramo` the engine and the oracle
# agree bit-exactly, which is a real datum about where the default-path gap
# lives (see the note in tools/automdl_scouting.md).
series{
  title  = "Amusements, gambling and recreation (CES, NSA)"
  file   = "../data/ces_amuse.dat"
  start  = 1990.01
  period = 12
}
transform{
  function = log
}
automdl{ noautooutlier = tramo }
estimate{ }
