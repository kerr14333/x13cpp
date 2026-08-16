# Hand-authored -- NOT produced by genextra.py; do not regenerate this directory.
# coverage: mkback.f:285-289's "User-defined prior adjustment factor not
# provided / for the backcast period." WARNING -- the BACKCAST twin of
# prtfct.f:485-489. It needs three things at once, which is why no spec had it:
#   * user prior factors (transform{data=}) -> Nustad > 0;
#   * a backcast window they do NOT cover (the factors start with the series,
#     maxback=12 puts Begbak a year earlier) -> lpria false;
#   * Prttab(LFORBC) or Savtab(LFORBC), whose deftab entry is FALSE -- hence
#     the explicit forecast{print=all}. The forecast-window twin needs none of
#     that, because deftab(LFOROS) is TRUE.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
  data = (
    100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0
    100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0
    100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0
    100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0
    100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0
    100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0
    112.5 112.5 112.5 112.5 112.5 112.5 100.0 100.0 100.0 100.0 100.0 100.0
    100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0
    100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0
    100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0
    100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0
    100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0 100.0
  )
  type = temporary
  mode = percent
  start = 1949.01
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
}
forecast{
  maxlead = 12
  maxback = 12
  print = all
  save = (ftr fvr fct btr bct)
}
x11{
  save = (d10 d11 d12 d13)
}
