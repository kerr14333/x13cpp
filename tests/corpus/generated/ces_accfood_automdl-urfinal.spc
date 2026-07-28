# Second automdl{urfinal=} gate, on a monthly series with a much larger blast
# radius than ukgas (measured oracle on-vs-off: 66 .udg keys move, against 33).
# BLS CES accommodation-and-food-services employment, NSA -- see
# ../data/ces_PROVENANCE.md for how the probe series were chosen.
series{
  title  = "CES accommodation and food services, employment (NSA)"
  file   = "../data/ces_accfood.dat"
  start  = 1990.01
  period = 12
}
transform{
  function = log
}
automdl{ urfinal = 1.50 }
estimate{ }
