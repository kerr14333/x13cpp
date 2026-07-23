# x11{calendarsigma=select sigmavec=(...)}: per-period sigma-limit selection
# (getx11.f:390-427 -> Csigvc; xtrm honours it when Ksdev==4).
series{ title="airline" file="../data/airline.dat" start=1949.01 period=12 save=(b1) }
x11{ calendarsigma=select sigmavec=(jan feb dec) save=(d10 d11 d12 d13) }
