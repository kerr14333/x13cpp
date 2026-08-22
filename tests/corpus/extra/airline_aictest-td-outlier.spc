# Hand-authored (NOT produced by genextra.py -- do not expect a
# regeneration to recreate it).
# coverage: regression{aictest=(td)} + outlier{} on an EXPLICIT arima{} model --
#           arima.f's :569 AIC-regressor-test arm followed by the :723 idotlr
#           call, which sits AFTER the :569/:701 if-else closes at :718 and so
#           fires on BOTH arms. run_pre_model.cpp's explicit_aictest arm is a
#           sibling `else if` of the plain-rgarma arm that carries the idotlr
#           call, so on this spec the port identifies no outlier at all.
#           No automdl{}/pickmdl{} on purpose: those take their own arms with
#           their own outlier identification, so this is the cheapest spec that
#           isolates the explicit-model arm. See tools/rx13cpp_engine_findings.md
#           (RX-1) for the measurement this was written from.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{
  function = log
}
regression{
  aictest = (td)
}
outlier{ }
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
