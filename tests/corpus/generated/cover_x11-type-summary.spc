# x11{type=summary}: summary adjustment (getx11.f:339-343 -> Kfulsm=1). The
# X-11 spine skips the final SA-round steps that Kfulsm<2 guards.
series{ title="airline" file="../data/airline.dat" start=1949.01 period=12 save=(b1) }
x11{ type=summary save=(d10 d11 d12 d13) }
