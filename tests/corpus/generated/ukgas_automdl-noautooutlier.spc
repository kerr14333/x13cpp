# Gates automdl{noautooutlier=tramo}: which arm of automd.f's label 40 runs.
#
# Lidotl is true by DEFAULT -- gtinpt.f:238 defaults Lotmod on, and automd.f:
# 175-181 turns that into a dummy AO scan with Critvl(AO)=BIGCV, which finds
# nothing but still routes label 40 to amidot. `noautooutlier=tramo` clears
# Lotmod, so with no outlier{} spec Lidotl is false and label 40 takes its
# `ELSE IF(.not.ismd0)` arm instead: tstmd1, which reverts to the airline
# default when the identified model's coefficients are not significantly
# better, then re-runs the regressor AIC tests.
#
# Reachable only because automd.cpp now has ONE path. tstmd1 reads three things
# this driver used to compute solely on its aictest branch: tair (automd.f:343,
# and note the oracle GUARDS that armats call on .not.Lidotl), adj0/trns0
# (:369-371), and bkdfmd's backup (:379, restored by tstmd1.f:221).
#
# ukgas is the QUARTERLY probe (Sp==4 changes tstmd1's own default-model test,
# tstmd1.f:47-57). Measured oracle on-vs-off: 20 .udg keys move, including
# every ARMA coefficient, both roots and durbinwatson.
series{
  title  = "UK quarterly gas consumption (NSA)"
  file   = "../data/ukgas.dat"
  start  = 1960.1
  period = 4
}
transform{
  function = log
}
automdl{ noautooutlier = tramo }
estimate{ }
