# Second automdl{noautooutlier=tramo} gate -- monthly, and a much larger blast
# radius than ukgas (measured oracle on-vs-off: 56 .udg keys move, against 20).
# See ukgas_automdl-noautooutlier.spc for what the argument selects, and
# ../data/ces_PROVENANCE.md for how the probe series were chosen.
series{
  title  = "Accommodation and food services (CES, NSA)"
  file   = "../data/ces_accfood.dat"
  start  = 1990.01
  period = 12
}
transform{
  function = log
}
automdl{ noautooutlier = tramo }
estimate{ }
