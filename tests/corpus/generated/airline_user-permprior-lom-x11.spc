# transform{ adjust=lom type=permanent data=(...) }: user permanent prior
# COMBINED with a predefined length-of-month prior (Priadj>1) -- exercises the
# x11pt2 makadj user branch + tdlom compose, and rmpadj (predef+user).
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
  save   = (b1)
}
transform{ function=log adjust=lom type=permanent data=(
    0.97 0.98 0.99 1.00 1.01 1.02 1.03 0.97 0.98 0.99 1.00 1.01
    1.02 1.03 0.97 0.98 0.99 1.00 1.01 1.02 1.03 0.97 0.98 0.99
    1.00 1.01 1.02 1.03 0.97 0.98 0.99 1.00 1.01 1.02 1.03 0.97
    0.98 0.99 1.00 1.01 1.02 1.03 0.97 0.98 0.99 1.00 1.01 1.02
    1.03 0.97 0.98 0.99 1.00 1.01 1.02 1.03 0.97 0.98 0.99 1.00
    1.01 1.02 1.03 0.97 0.98 0.99 1.00 1.01 1.02 1.03 0.97 0.98
    0.99 1.00 1.01 1.02 1.03 0.97 0.98 0.99 1.00 1.01 1.02 1.03
    0.97 0.98 0.99 1.00 1.01 1.02 1.03 0.97 0.98 0.99 1.00 1.01
    1.02 1.03 0.97 0.98 0.99 1.00 1.01 1.02 1.03 0.97 0.98 0.99
    1.00 1.01 1.02 1.03 0.97 0.98 0.99 1.00 1.01 1.02 1.03 0.97
    0.98 0.99 1.00 1.01 1.02 1.03 0.97 0.98 0.99 1.00 1.01 1.02
    1.03 0.97 0.98 0.99 1.00 1.01 1.02 1.03 0.97 0.98 0.99 1.00
) }
arima{ model=(0 1 1)(0 1 1) }
estimate{ }
x11{ save = (d10 d11 d12 d13) }
