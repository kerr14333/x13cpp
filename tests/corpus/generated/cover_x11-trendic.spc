# x11{trendma + trendic}: fixed Henderson length + I/C ratio (getx11.f -> Ktcopt,
# Tic; vtc). trendic requires a fixed trendma (else the oracle errors).
series{ title="airline" file="../data/airline.dat" start=1949.01 period=12 save=(b1) }
x11{ trendma=23 trendic=4.5 save=(d10 d11 d12 d13) }
