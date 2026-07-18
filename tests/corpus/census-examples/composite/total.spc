# Composite example — the aggregate ("total") spec.
# This spec uses a composite{} spec INSTEAD of a series{} spec: its data is
# the sum of the component series (region_north + region_south) formed by the
# program when the three specs are run together through composite.mta.
# The composite total is then indirectly/directly seasonally adjusted here.
composite{
  title  = "Total Sales (North + South)"
}
x11{ }
