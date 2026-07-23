# x11{shrink=global}: global shrinkage of the seasonal factors (getx11.f:459-465
# -> Ishrnk=1; shrink.f/glbshk.f, Miller & Williams 2003).
series{ title="airline" file="../data/airline.dat" start=1949.01 period=12 save=(b1) }
x11{ shrink=global save=(d10 d11 d12 d13) }
