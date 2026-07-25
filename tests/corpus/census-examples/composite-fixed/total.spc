# Composite gate — the aggregate ("total") spec.
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
          ie8 ip8 iee i18 ita)
}
x11{
  save = (d10 d11 d12 d13)
}
