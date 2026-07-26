# coverage: regression{b=(.. f)} -- FIXED regression coefficients, the whole
# chain from gtrgvl.f's parse through regfix.f's Iregfx to arima.f:281-287's
# rmfix (strike the fixed columns from the design, subtract b*X from the series,
# estimate the residual model) and arima.f:907-914's addfix restore.
#
# The b= argument was parsed and SILENTLY DROPPED: gt_regression had no branch
# for it at all, so the coefficients were estimated as if the user had said
# nothing, behind an OUTCOME: OK. Measured oracle on-vs-off (same spec without
# b=): 1.1e-2 in d10/d11/d16, 2.3e-2 in d12, 2.5e-2 in d13.
#
# Both coefficients fixed, so Iregfx==3 (regfix.f's allfix branch) and NOTHING
# in the regression is estimated -- only the ARMA. See airline_regb-mixed for
# the Iregfx==2 partial strip, which is the more delicate rmfix path.
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
  b = (0.05f, -0.04f)
}
arima{ model=(0 1 1)(0 1 1) }
estimate{ }
x11{ save=(d10 d11 d12 d13 d16) }
