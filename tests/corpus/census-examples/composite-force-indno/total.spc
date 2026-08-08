# Composite gate — the aggregate, with force{indforce=no}.
#
# The other arm of agr3.f:436: with indforce=no the indirect forced series is NOT
# benchmarked, it is the AGGREGATE of the components' own forced SA series
# (agr3.f:476 copies Ci2, which agr2.f:277 summed from each component's Stci2).
# So the force{} here has to be on the COMPONENTS as well -- with it only on the
# total, Ci2 is the zero buffer and `iaa` comes out zero, which is a real oracle
# behaviour but not the one this case is for.
#
# No round=yes: that half is gated by ../composite-force/, and leaving it off
# keeps agr3.f:537's [ib,ie] ftest out of this case, so the two specs discriminate
# the two savelog writers instead of both landing on the last one.
#
# ONLY THE NORTH COMPONENT IS FORCED, and that asymmetry is the whole reason
# this case gates anything. Benchmarking commutes with the sum: with both
# components carrying the same force{}, the ORACLE's iaa is the same to
# 4.9e-15 whether indforce is yes or no -- for type=denton, which is linear
# in the annual discrepancies, and measured to be so for type=regress too.
# A mutation that forces the benchmarking arm therefore measured ZERO on the
# whole suite. Drop force{} from the south component and the arms separate by
# 1.4e-05: the Ci2 sum now carries one forced series and one untouched
# Stci2 buffer, which no benchmarking of the aggregate can reproduce.
#
# type=regress (qmap2) rather than denton, so the composite path exercises
# the Cholette-Dagum branch that ../composite-force/ does not.
#
# Uses a composite{} spec INSTEAD of series{}: its data is the sum of the
# component series, formed by the program when all three specs run together
# through composite.mta (metafile mode). The total is then adjusted BOTH ways:
#   x11{ save=(d10..d13) }        -- DIRECT: the aggregate adjusted in its own right
#   composite{ save=(isf isa itn iir) } -- INDIRECT: rebuilt from the component results
composite{
  title  = "Total Sales (North + South)"
  # The INDIRECT diagnostics family as well: the D8/D9 SI ratios and the
  # Part-E tables x11ari.f:341's second x11pt4 pass produces.
  save = (isf isa itn iir id8 id9 ie1 ie2 ie3 ie5 ip5 ie6 ip6 ie7 ip7
          ie8 ip8 iee i18 ita iaa iff)
}
x11{
  save = (d10 d11 d12 d13)
}
force{
  type     = regress
  indforce = no
}
