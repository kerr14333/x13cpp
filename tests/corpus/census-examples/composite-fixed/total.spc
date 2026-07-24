# Composite gate — the aggregate ("total") spec.
#
# Uses a composite{} spec INSTEAD of series{}: its data is the sum of the
# component series, formed by the program when all three specs run together
# through composite.mta (metafile mode). The total is then adjusted BOTH ways:
#   x11{ save=(d10..d13) }        -- DIRECT: the aggregate adjusted in its own right
#   composite{ save=(isf isa itn iir) } -- INDIRECT: rebuilt from the component results
composite{
  title  = "Total Sales (North + South)"
  save = (isf isa itn iir)
}
x11{
  save = (d10 d11 d12 d13)
}
