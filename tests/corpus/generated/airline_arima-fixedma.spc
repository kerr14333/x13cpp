# coverage: arima{ma=(.. f)} -- FIXED ARMA coefficients (gtinvl.f), the ARIMA
# twin of regression{b=}. Same silent-drop class: gt_arima consumed the diff/ar/
# ma argument values and threw them away, so the model was estimated freely and
# the run came back OUTCOME: OK with the wrong coefficients. Measured oracle
# on-vs-off: 6.2e-3 in d10/d11/d12/d16, 7.9e-3 in d13. (The oracle's printed
# roots confirm the fix took: 1/0.55 = 1.8182 and 1/0.35 = 2.8571.)
#
# mdlfix.f -- which derives Imdlfx from Arimap/Arimaf exactly as regfix.f does
# for the regression -- was already ported and already called; it simply had
# nothing to read. Imdlfx>=1 is also what estimate{parms=fixed} requires, so
# that argument was unusable before this too.
#
# Both MA lags fixed => Imdlfx==3, and rgarma has no free ARMA parameter left;
# only the regression (here empty) would be estimated.
# Hand-authored; NOT produced by genspecs.py.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
  save   = (b1)
}
transform{ function=log }
arima{
  model = (0 1 1)(0 1 1)
  ma    = (0.55f, 0.35f)
}
estimate{ }
x11{ save=(d10 d11 d12 d13 d16) }
