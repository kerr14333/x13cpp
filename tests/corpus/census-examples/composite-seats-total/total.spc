# SEATS composite gate -- the aggregate ("total") adjusted by SEATS too.
#
# The sibling of ../composite-seats/, whose total is X-11 adjusted. This one
# flips agr3s.f's `Lx11` argument to false, and it is the only corpus case in
# which the composite TOTAL goes through run_seats -- i.e. in which the whole
# x11ari.f:329-374 composite tail is reached from the SEATS driver rather than
# the X-11 one.
#
# What the flip is NOT: a change to any emitted value. Lx11 selects Stc/Ci vs
# Seattr/Seatsa for agr3s's `Tem` stash, and `Tem` is the DIRECT trend that
# agr2's R2 half reads -- which !X11agr has already dropped. Measured: r1mse
# 22.448/23.732 and r1rmse 11.936/12.668 here, identical to ../composite-seats/.
# The Stci write beside it is overwritten with Ci two screens later either way.
#
# A composite{} total carrying seats{} needs a model of its own; without one the
# oracle refuses the spec outright ("A spec that requires modeling was found ...
# but no provision for an ARIMA model"). Fixed coefficients for the same reason
# as the components -- see region_north.spc.
#
# Hand-authored; NOT produced by any generator.
composite{
  title = "Total Sales (North + South)"
  save = (isf isa itn iir ie5 ip5 ie6 ip6 i18 ita)
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
  ma    = (0.55f, 0.35f)
}
estimate{ }
seats{
  save = (s10 s11 s12 s13)
}
