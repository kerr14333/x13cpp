# Second series for automdl{diff=} -- monthly, and a different default
# differencing (0,1) from the given (1,1), so the mdlset branch is observable.
series{
  title  = "Nottingham monthly temperature (NSA)"
  file   = "../data/nottem.dat"
  start  = 1920.01
  period = 12
}
transform{
  function = none
}
automdl{ diff = (1,1) }
estimate{ }
