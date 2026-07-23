series{ title="airline" file="../data/airline.dat" start=1949.01 period=12 }
transform{ function=log }
regression{ aictest=(td) aicdiff=0.0 }
arima{ model=(0 1 1)(0 1 1) }
estimate{ }
