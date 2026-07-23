# x11{type=trend}: trend adjustment (getx11.f:339-343 -> Kfulsm=2). The X-11
# spine takes the trend-only branches guarded by Kfulsm==2.
series{ title="airline" file="../data/airline.dat" start=1949.01 period=12 save=(b1) }
x11{ type=trend save=(d10 d11 d12 d13) }
