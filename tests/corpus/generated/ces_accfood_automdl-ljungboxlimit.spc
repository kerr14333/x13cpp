# Second automdl{ljungboxlimit=} gate -- 90 .udg keys move at 0.5, and unlike
# nottem this series also moves at 0.75 and even at 0.99, so it is the one that
# pins the Pcr increment across more than one nloop pass.
# See nottem_automdl-ljungboxlimit.spc for what the argument does.
series{
  title  = "Accommodation and food services (CES, NSA)"
  file   = "../data/ces_accfood.dat"
  start  = 1990.01
  period = 12
}
transform{
  function = log
}
automdl{ ljungboxlimit = 0.5 }
estimate{ }
