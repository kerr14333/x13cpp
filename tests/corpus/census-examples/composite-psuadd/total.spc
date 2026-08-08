# Composite gate — the aggregate, PSEUDO-ADDITIVE.
#
# agr3.f:267-272, the last unported piece of composite{}. The multiplicative
# ELSE arm twenty characters away was ported with the rest of agr3 and the
# Psuadd one was not, so a pseudo-additive composite returned OUTCOME: OK with a
# wrong `isf` and no `isd` at all.
#
# The measurement that makes this spec worth its bytes: with mode=pseudoadd,
# EVERY other indirect table is already bit-exact -- d10-d13, isa, itn, iir,
# id8, ie1, i18 all at ~5e-15 -- and `isf` alone was 1.6e-06 out. The two
# formulas are close, which is exactly how an unported arm survives. `isd` (the
# seasonal DIFFERENCES, table D10B) exists only on this arm and was absent.
#
# Note what had to change besides the mode: transform{function=log} is GONE from
# all three specs. Pseudo-additive and a log are incompatible, so the comparison
# arm for the on-vs-off measurement is this corpus with mode=mult -- not
# ../composite-fixed/, which would have been measuring the transform.
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
          ie8 ip8 iee i18 ita isd)
}
x11{
  mode = pseudoadd
  save = (d10 d11 d12 d13)
}
