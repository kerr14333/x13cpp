# NOT produced by gen*.py -- hand-authored; do not delete when regenerating.
# coverage: history{endtable=} (Rvend -> Endsa/Endtbl/Revnum, setrvp.f:21-26).
#
# This spec exists because the feature was ALREADY CORRECT and nothing gated it.
# tools/history_options_scouting.md listed endtable= among the parsed-but-silent
# flags on the strength of an ORACLE on-vs-off measurement ("rows 71->48 and
# values 1.1e-4") -- which proves the flag matters, NOT that the engine ignores
# it. Nobody had run engine-vs-oracle. It turns out gt_history case 6 parses it
# into rv.rvend and run_history already derives endsa/endtbl/revnum from it, so
# the row count and every retained value match. Measured with endtable=1957.dec:
# the oracle and the engine both emit 36 rows instead of 71, at the same
# per-span floor as the full-table run.
#
# The two revchk.f pieces around it were checked and need no port:
#   * revchk.f:616-624, the `.not.Revsa` warning-and-reset (endtable= given but
#     no sadj/seasonal/trend estimate requested -> Rvend forced back to the end
#     of the series), is observationally INERT. Nothing that survives that
#     branch reads Endtbl: the model histories lkh/amh/tdh use Begrev..Endrev
#     and the forecast history keys on Revptr+Rfctlg. Measured with
#     estimates=(aic arma)+endtable and estimates=(fcst)+endtable: oracle and
#     engine agree on the row counts either way (72 and 71).
#   * revchk.f:629's `IF(Irev.eq.2)` override is DEAD CODE -- Irev is only ever
#     set to 0, 1 (gtrvst.f:349) or 4 (revdrv.f:387) anywhere in the Fortran.
#
# So do not "port" endtable=; just keep this gate, which is what would catch a
# regression in the endtbl arithmetic.
series{
  title = "International Airline Passengers"
  file = "../data/airline.dat"
  start = 1949.01
  period = 12
}
transform{
  function = log
}
arima{
  model = (0 1 1)(0 1 1)
}
estimate{
  print = all
  savelog = all
}
x11{
  print = all
  savelog = all
}
history{
  estimates = (sadj sadjchng seasonal trend trendchng)
  start = 1955.jan
  endtable = 1957.dec
  print = all
  save = (sar sae chr che trr tre tcr tce sfr sfe)
  savelog = all
}
