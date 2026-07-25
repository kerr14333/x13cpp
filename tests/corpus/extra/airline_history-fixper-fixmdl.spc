# coverage: the Fixper x Revfix INTERACTION (revchk.f:801-805). With the model
# already held fixed by history{fixmdl=yes}, the "0.per" re-estimate-once-a-year
# convention has nothing left to do, so Fixper is reset to 0 and the per-span
# Endmdl capping does NOT happen -- while series{modelspan=(,0.jan)} still
# narrows the MAIN run's own estimation, which is why this differs from plain
# fixmdl (measured oracle delta vs airline_history-fixmdl: sae 1.15e-3,
# tre 2.83e-3, sfe 1.22e-3). Gates bit-exact (~5e-15) like its fixmdl sibling.
# Hand-authored; NOT produced by gen*.py.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
  modelspan = ( , 0.jan)
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  print = all
  savelog = all
}
x11{
  print = all
  savelog = all
}
history{
  estimates = (sadj sadjchng seasonal trend trendchng)
  start = 1955.jan
  fixmdl = yes
  print = all
  save = (sar sae chr che trr tre tcr tce sfr sfe)
  savelog = all
}
