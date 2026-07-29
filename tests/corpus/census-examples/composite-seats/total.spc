# SEATS composite gate -- the aggregate ("total") spec.
#
# The components are SEATS-adjusted, so X11agr is false and x11ari.f:342 routes
# the indirect adjustment through agr3s.f. The total ITSELF is still adjusted by
# X-11, which is what makes agr3s's `Lx11` argument true here and gives the run a
# DIRECT d10-d13 to compare the indirect tables against.
#
# The requested save list is deliberately the FULL indirect family, most of which
# the oracle then does not write: agr3s produces no indirect trend or irregular,
# no D8/D9 pair and no x11pt4 pass, so itn/iir/id8/id9/ie1/ie2/ie3/ie7/ip7/ie8/
# ip8/iee/ita simply do not exist on this path. Asking for them and getting
# nothing is the assertion -- the gate checks BOTH directions.
composite{
  title = "Total Sales (North + South)"
  save = (isf isa itn iir id8 id9 ie1 ie2 ie3 ie5 ip5 ie6 ip6 ie7 ip7
          ie8 ip8 iee i18 ita)
}
x11{
  save = (d10 d11 d12 d13)
}
