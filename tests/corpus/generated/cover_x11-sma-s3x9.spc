# x11{seasonalma=s3x9}: explicit seasonal-filter selection (getx11.f:228-268 ->
# Lterm/Lter, honoured by vsfa). No-model direct-X11 path.
series{ title="airline" file="../data/airline.dat" start=1949.01 period=12 save=(b1) }
x11{ seasonalma=s3x9 save=(d10 d11 d12 d13) }
