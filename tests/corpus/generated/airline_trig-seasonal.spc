# Hand-authored parity spec (NOT genspecs.py output): gates the trigonometric
# (sine-cosine) seasonal regressors (regression variables=(sincos[...]) -> the
# adsncs.f builder, regvar.f case 30). Harmonics 1,2,3 give 6 columns
# cos/sin(2pi*h t/12); the regARIMA fit estimates their 6 coefficients. Model is
# nonseasonal (0 1 1): the trig regressors carry the seasonality.
series{
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
}
regression{
  variables = (sincos[1 2 3])
}
arima{
  model = (0 1 1)
}
estimate{
  save = (est)
  savelog = all
}
forecast{
  maxlead = 12
}
x11{ }
