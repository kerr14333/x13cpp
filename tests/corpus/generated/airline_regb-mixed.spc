# coverage: regression{b=} with a MIX of fixed and estimated coefficients, i.e.
# regfix.f's Iregfx==2. This is the delicate half of the rmfix seam: only the
# columns whose Regfx is set are struck from the design (rmfix.f's
# `Regfx(icol) .or. Fxindx==2` test), the rest are estimated alongside the ARMA,
# and addfix has to re-insert the struck ones in the right GROUP position --
# which for an outlier is by DATE, not appended (adrgef.f:82-126).
#
# Iregfx==3 (airline_regb-fixed) never exercises the partial strip, and it also
# never exercises adrgef's date-ordered re-insertion with a surviving group
# already present. Measured oracle on-vs-off: 7.7e-3 in d10/d11/d16, 1.5e-2 in
# d13.
# Hand-authored; NOT produced by genspecs.py.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
  save   = (b1)
}
transform{ function=log }
regression{
  variables = (ao1955.jan ls1960.jul)
  b = (0.05f, -0.04)
}
arima{ model=(0 1 1)(0 1 1) }
estimate{ }
x11{ save=(d10 d11 d12 d13 d16) }
