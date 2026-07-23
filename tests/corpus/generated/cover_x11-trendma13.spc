# x11{trendma=13}: fixed Henderson trend-filter length (getx11.f:283-295 ->
# Ktcopt, honoured by vtc).
series{ title="airline" file="../data/airline.dat" start=1949.01 period=12 save=(b1) }
x11{ trendma=13 save=(d10 d11 d12 d13) }
