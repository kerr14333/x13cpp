# x11{shrink=local}: local shrinkage of the seasonal factors (getx11.f:459-465
# -> Ishrnk=2; shrink.f/locshk.f/lkshnk.f, Miller & Williams 2003).
series{ title="airline" file="../data/airline.dat" start=1949.01 period=12 save=(b1) }
x11{ shrink=local save=(d10 d11 d12 d13) }
