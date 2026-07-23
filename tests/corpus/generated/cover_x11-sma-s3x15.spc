# x11{seasonalma=s3x15}: explicit seasonal filter (getx11.f -> Lterm/Lter via vsfa).
series{ title="airline" file="../data/airline.dat" start=1949.01 period=12 save=(b1) }
x11{ seasonalma=s3x15 save=(d10 d11 d12 d13) }
