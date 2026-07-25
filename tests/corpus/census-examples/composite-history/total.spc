# Composite INDIRECT revision-history gate -- the aggregate ("total") spec.
#
# Its own history{} produces the DIRECT revisions of the aggregate (sar/sae) and,
# because it is the run where agr2 has moved Iagr to 5, ALSO prints the INDIRECT
# revision table the components accumulated: iar (Ind_SA_revision) and iae
# (Conc_Ind_SA, Final_Ind_SA), plus the `historyindsa: yes` savelog canary.
#
# Hand-authored; NOT produced by gen*.py.
composite{
  title  = "Total Sales (North + South)"
  save = (isf isa itn iir)
}
x11{
  save = (d10 d11 d12 d13)
}
history{
  estimates = (sadj)
  start = 1996.jan
  save = (sar sae iar iae)
  savelog = all
}
