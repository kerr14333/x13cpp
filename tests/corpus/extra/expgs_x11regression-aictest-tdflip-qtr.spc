# Hand-authored (NOT produced by genextra.py).
# coverage: editor.f:1786's Grpx(-1) read -- the ONE consequence of it that
# lands at PARSE time, and therefore the only way to gate the read itself
# without also gating the auto-AO AICC gap it is inseparable from.
#
# THE READ. `aictest=(td)` names a trading day that `variables=` does not, so
# Tdgrp==0 and editor.f:1786's `begcol=Grpx(Tdgrp-1)` is `Grpx(-1)` -- one
# element below the declared lower bound of `Grpx(0:PGRP)`. It is NOT undefined:
# xrgmdl.cmn:49 declares `Clxptr(0:PB)` immediately before `Grpx`, and Fortran
# storage association makes a COMMON block contiguous in declaration order, so
# the subscript resolves to `Clxptr(PB)`, 81 integers INSIDE the block. Proved
# by tools/ref_grpx.f. `Clxptr(PB)` is `Colptr(PB)` (loadxr.f:38 copies all
# PB+1 elements), which no model under 79 regressors ever writes, so it holds
# the block's static 0; `Grpx(0)` is 1, `endcol` is 0, they compare equal, and
# Xtdtst flips 1 -> 3. `td` has silently become `td1coef`. CB-37.
#
# WHY QUARTERLY. On monthly data the flip is invisible at parse time -- it only
# changes which regressor the AIC test scores, and that lands in AICCs this port
# still walls (the auto-AO half). On QUARTERLY data the rewritten value walks
# into editor.f:1832's `(Xtdtst.eq.3.or.Xtdtst.eq.4).and.Sp.ne.12` and the run
# is REFUSED. Without the flip Xtdtst is still 1, that arm does not fire, and
# Sp==4 passes the next arm cleanly -- so this refusal cannot happen unless the
# aliased read happened. That is what makes it a gate for the read and not for
# anything downstream of it.
#
# Note the message: it says "stock trading day" for a plain `td` request, and
# names a condition (3 or 4 -- the two "1coef" flavours) that has nothing to do
# with stock TD. Reproduced verbatim, not corrected.
#
# Gated by tests/parity/test_m1_parse.py::test_outcome_matches_oracle, which
# compares the ERROR text against the blessed oracle .err line for line.
series{
  title = "Quarterly expenditures on goods and services"
  file = "../data/expgs.dat"
  start = 1947.1
  period = 4
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  savelog = all
}
x11{
  save = (d10 d11 d12 d13)
  savelog = all
}
x11regression{
  variables = (easter[8])
  aictest = (td)
  print = all
}
