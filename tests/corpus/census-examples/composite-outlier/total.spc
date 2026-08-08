# Composite gate — the aggregate, with an OUTLIER on a component.
#
# The spec that makes `Lindot` observable. gtinpt.f:311 defaults
# composite{indoutlier=} to YES; this port never wrote the flag anywhere, so it
# was false on every run and all four guards keyed on it were dead code --
# agr3.f:200's indirect outlier-factor build, :222's level-shift refold into the
# published trend, :227's AO factor and :298's D8 divide.
#
# Nothing could see it while no composite in the corpus carried an outlier:
# Lindls/Lindao are false without one, and every consumer is a conjunction with
# Lindot, so the two readings agree. Put a level shift on ONE component and
# itn/iir/id8/id9 came back 3.5e-04 out at OUTCOME: OK. Same shape as entry 87's
# Irev -- nothing refuses, nothing is walled, and the guards look ported.
#
# North carries the shift and south does not, deliberately: the indirect trend
# is the aggregate of the components, so an asymmetric design keeps the refold
# from cancelling.
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
