# Gates automdl{balanced=yes}: prefer models whose combined AR+differencing
# order equals the combined MA order (the TRAMO preference). ukgas is the only
# corpus series whose top-two BIC gap (0.001) is narrow enough for the
# preference to flip the choice.
series{
  title  = "UK quarterly gas consumption (NSA)"
  file   = "../data/ukgas.dat"
  start  = 1960.1
  period = 4
}
transform{
  function = log
}
automdl{ balanced = yes }
estimate{ }
