# Hand-authored (2026-08-10). x11regression{holidaynonlin=yes} -> Xhlnln
# (gtxreg.f:442-449), on the arm where rgtdhl.f RETURNS.
#
# Xhlnln is the Bell-Hilmer nonlinear Easter. rgtdhl.f keeps its own guard
# INSIDE the routine, deliberately (rgtdhl.f:37-40: "so that the call from the
# outlier identification routines is kept as clean as possible"), and it needs
# ALL of: Xhlnln, not pseudo-additive, Muladd==0, Easidx==0, a HOLIDAY group
# with .not.Axruhl, and a trading-day group. This spec sets the option and
# fails exactly one clause -- the x11regression design is trading-day only, so
# Holgrp is 0 -- and the oracle returns from rgtdhl immediately. Measured: 0
# output lines differ from the same spec with holidaynonlin=no.
#
# That is what this spec gates: the port's wall is keyed on rgtdhl's OWN
# condition, not on the option. The first version of it refused at the heads of
# x11aic/x11mdl_td the moment Xhlnln was set, which would have turned this run
# -- one the oracle completes -- into a refusal. Setting an option is not the
# same as reaching the code it enables.
#
# The arm that DOES run is walled and NOT gated: add a holiday group back and
# the oracle's nonlinear design comes out singular, prterx halts it, and it
# does so on airline, ces_accfood, ces_leis and expgs, with easter[8] and
# easter[15] alike. The other route out -- x11{mode=add}, which clears Muladd
# -- walks into a DIFFERENT wall this port already has (x11pt1's
# additive/pseudo-additive prior trading day). So no spec this corpus can build
# both reaches rgtdhl's body and finishes, and the wall carries a measurement
# rather than a gate. See docs/M5_PORT_NOTES.md entry 105.
series{
  title = "Airline"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{ function = log }
arima{
  model = (0 1 1)(0 1 1)
}
estimate{ }
x11regression{
  variables = (td)
  holidaynonlin = yes
}
x11{ save = (d10 d11 d12 d16) }
