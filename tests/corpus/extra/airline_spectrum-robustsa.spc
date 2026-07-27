# coverage: spectrum{robustsa=no} (Lrbstsa) -- which SA series and which
# irregular the spectrum diagnostics are taken OF. The default (yes) uses the
# extreme-value-modified Part-E pair E2/E3; `no` uses D11/D13 straight
# (spcdrv.f:304/316/441/447).
#
# The argument was in gt_spectrum's ARGDIC and had no parse branch, so it was
# accepted and silently dropped -- the engine always took the E2/E3 arm. It
# needs its own spec because no other corpus spec sets it, and the default
# leaves both arms of that branch unreachable.
#
# The OUTLIER REGRESSORS are load-bearing, not decoration. x11pt3 folds the
# AO/TC back into the PUBLISHED D13 and the LS into the PUBLISHED D12 while the
# oracle keeps /x11srs/ Sti and Stc internal; this port mirrors the published
# values back over them so the harness can punch d12/d13 off the COMMON. spcdrv
# runs AFTER x11pt3, so its D13 is the INTERNAL one -- and without an outlier
# there is nothing to fold and the two are identical, i.e. a robustsa=no spec
# with no outliers would gate green through the published buffer. Measured with
# the published D13: spcirr.median -44.497 against the oracle's -37.352, and 28
# of the 86 spectrum savelog lines wrong.
#
# Hand-authored; NOT produced by genextra.py.
series{
  title  = "Intl Airline Passengers"
  file   = "../data/airline.dat"
  start  = 1949.01
  period = 12
}
transform{ function = log }
regression{ variables = (ao1955.jan tc1957.mar ls1960.jul) }
arima{ model = (0 1 1)(0 1 1) }
estimate{ }
spectrum{ robustsa = no }
x11{ save = (d11 d12 d13) }
