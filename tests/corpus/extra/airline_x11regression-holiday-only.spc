# Hand-authored (NOT produced by genextra.py -- do not delete when regenerating).
# coverage: an x11regression{} whose ONLY regressor is a holiday. No trading-day
# group, so editor.f:1722 clears Axrgtd and Axrghl is the only flag left saying
# the irregular regression has a prior to estimate.
#
# The corpus had no such spec for a long time, and its absence hid a whole
# skipped phase. FOUR guards in this port were keyed on Axrgtd where the oracle
# keys on something wider, and every one of them is false here:
#
#   x11ari.f:91   -- `IF(Ixreg.eq.2.or.Khol.eq.1) CALL xrgdrv`. The port tested
#                    Axrgtd, at BOTH call sites (run_pre_model, x11_prestage).
#   xrgdrv.f      -- the port's own entry test, which RETURNED TRUE and did
#                    nothing rather than fatal. A silent no-op, which is why it
#                    survived: an unported path that stops is visible.
#   x11pt1        -- the compensating restore of the stashed prior Faccal (a
#                    port artifact; the oracle's COMMON just survives).
#
# So the transparent prior pass never ran: B1 came back as the RAW series where
# the oracle had already divided the Easter factor out (8.8e-3), the automatic
# AO identification in the irregular regression found nothing where the oracle
# keeps AO1960.Mar, and the seasonal filter choice flipped (3x3 vs 3x5, D7 trend
# MA 9 vs 13) -- all at OUTCOME: OK.
#
# Two more defects sat underneath, both invisible until the pass ran at all:
#   - Easgrp was READ (x11mdl.f derives Holgrp from it) and never WRITTEN; the
#     editor computed it into a local and threw it away. With Holgrp 0 and no TD
#     group, x11mdl.f:308's identity-factor NOTE branch fired and the regression
#     was not fitted at all.
#   - x11ref.f's Tdgrp==0 arms were not ported. The Tdgrp>0 arm adds
#     Xn/Xnstar -- the month-length ratio -- to a factor that here is supposed to
#     be holiday-only, which put February off by exactly 28/28.25.
#
# gtxreg.f:889's `IF(Ixrghl.gt.0)Axrghl=T` had also been deliberately skipped;
# it is taken now, and no gated spec moved.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
  save = (b1)
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
  savelog = all
  save = (d10 d11 d12 d13 d16)
}
x11regression{
  variables = (easter[8])
  print = all
  save = (xrm bxh xhl)
}
