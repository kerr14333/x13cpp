# per-period seasonalma: different filters per month, exercising Lter[] (getx11.f:258-268).
series{ title="airline" file="../data/airline.dat" start=1949.01 period=12 save=(b1) }
x11{ seasonalma=(s3x3 s3x3 s3x5 s3x5 s3x9 s3x9 s3x9 s3x9 s3x5 s3x5 s3x3 s3x3) save=(d10 d11 d12 d13) }
