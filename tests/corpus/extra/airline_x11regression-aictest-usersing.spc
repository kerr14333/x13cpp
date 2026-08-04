# Hand-authored (NOT produced by genextra.py).
# coverage: prterx.f -- the irregular regression's SINGULAR-DESIGN abend, and the
# only spec in the corpus that reaches it. The oracle stops here; this engine used
# to print OUTCOME: OK on a seasonal adjustment whose calendar regression never
# fitted. See docs/M5_PORT_NOTES.md entry 73.
#
# `variables=(td)` with `aictest=(user)` and NOTHING else is the whole trick.
# x11aic.f runs three tests in order -- trading day (:148-297), Easter
# (:298-458), user (:462-591) -- and only the first two call regvar. With
# Xtdtst==0 and Xeastr==F both are skipped, so the `IF(estend) CALL regx11(A)`
# at :463-464 fits whatever design happens to be resident. Nothing built one:
# x11mdl.f's own first regvar is at :388, AFTER the x11aic call at :253, and
# x11pt2.f:720's `IF(Ixreg.eq.1) CALL loadxr(F)` has just restored Nrxy from
# Nxrxy -- parked by gtinpt.f:830's loadxr(T) at PARSE time, before any regvar
# ran. Ixreg stays 1 here (editor.f:1976-1978 promotes to 2 only for a holiday,
# a fixed prior, or a narrowed span; this spec has none), so xrgdrv never runs
# either. olsreg goes singular on column 1 and prterx names it: "Mon".
#
# TWO ROW COUNTS, and they are not the same number. prterx.f:52 reprints the
# design over Nrxy rows -- zero of them, hence the oracle's six column headers
# above an empty table -- while regx11.f:49-50 fits Nspobs of them. Reading the
# empty print as "the design regx11 saw" is the wrong inference; the singularity
# is in the CONTENT of Xy, not in a zero row count.
#
# What this spec pins is the abend itself: OUTCOME: FATAL plus prterx's exact
# two-line Mt2 text. It ships no table goldens because the oracle produces none.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  print = all
  savelog = all
}
x11{
  print = all
  save = (d10 d11 d12 d13 d16)
  savelog = all
}
x11regression{
  variables = (td)
  aictest = (user)
  user = (u1)
  data = (
    0.050000 0.049384 0.047553 0.044550 0.040451 0.035355
    0.029389 0.022700 0.015451 0.007822 0.000000 -0.007822
    -0.015451 -0.022700 -0.029389 -0.035355 -0.040451 -0.044550
    -0.047553 -0.049384 -0.050000 -0.049384 -0.047553 -0.044550
    -0.040451 -0.035355 -0.029389 -0.022700 -0.015451 -0.007822
    0.000000 0.007822 0.015451 0.022700 0.029389 0.035355
    0.040451 0.044550 0.047553 0.049384 0.050000 0.049384
    0.047553 0.044550 0.040451 0.035355 0.029389 0.022700
    0.015451 0.007822 0.000000 -0.007822 -0.015451 -0.022700
    -0.029389 -0.035355 -0.040451 -0.044550 -0.047553 -0.049384
    -0.050000 -0.049384 -0.047553 -0.044550 -0.040451 -0.035355
    -0.029389 -0.022700 -0.015451 -0.007822 0.000000 0.007822
    0.015451 0.022700 0.029389 0.035355 0.040451 0.044550
    0.047553 0.049384 0.050000 0.049384 0.047553 0.044550
    0.040451 0.035355 0.029389 0.022700 0.015451 0.007822
    0.000000 -0.007822 -0.015451 -0.022700 -0.029389 -0.035355
    -0.040451 -0.044550 -0.047553 -0.049384 -0.050000 -0.049384
    -0.047553 -0.044550 -0.040451 -0.035355 -0.029389 -0.022700
    -0.015451 -0.007822 0.000000 0.007822 0.015451 0.022700
    0.029389 0.035355 0.040451 0.044550 0.047553 0.049384
    0.050000 0.049384 0.047553 0.044550 0.040451 0.035355
    0.029389 0.022700 0.015451 0.007822 0.000000 -0.007822
    -0.015451 -0.022700 -0.029389 -0.035355 -0.040451 -0.044550
    -0.047553 -0.049384 -0.050000 -0.049384 -0.047553 -0.044550
    -0.040451 -0.035355 -0.029389 -0.022700 -0.015451 -0.007822
    0.000000 0.007822 0.015451 0.022700 0.029389 0.035355
  )
  start = 1949.01
  print = all
  save = (xrm b16 c16)
}
