# Composite gate — the aggregate, with force{} on the INDIRECT adjustment.
#
# Copy of ../composite-fixed/ plus a force{} on the total. It gates agr3.f:404-547,
# which had no port at all: agr3s.f's twin was ported with the SEATS branch and
# this X-11 one was not, so `force{}` on an X-11 composite total produced no
# iaa/iff/irn and returned OUTCOME: OK.
#
# What each option is here for:
#   type=denton  -- the Iyrt==1 arm, qmap plus the two partial-year fix-ups
#                   (agr3.f:449-470; the leading one runs `Posfob,ib-1` and is a
#                   Census no-op, transcribed).
#   round=yes    -- rndsa, and with it agr3.f:537's ftest over the QMAP OUTPUTS
#                   [ib,ie] rather than [Pos1ob,Posfob]. agr3s.f:328 passes the
#                   span there and additionally guards on Lx11; agr3.f does
#                   neither. That asymmetry is Census's and is what this spec
#                   pins.
#   usefcst      -- left at its default yes, which widens `iff` to the forecast
#                   span (192 rows against iaa/irn's 180) and is the only thing
#                   that makes lstfrc differ from Posfob.
#
# indforce is left at its DEFAULT (yes). ../composite-force-indno/ takes the
# other arm.
#
# Measured on the oracle, this spec against ../composite-fixed/: three save files
# appear, `indforce: yes`, and `id11.f` moves 0.02200 -> 0.87565 because the
# rounded/forced ftests overwrite the plain one under the same savelog key.
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
          ie8 ip8 iee i18 ita iaa irn iff)
}
x11{
  save = (d10 d11 d12 d13)
}
force{
  type  = denton
  round = yes
}
