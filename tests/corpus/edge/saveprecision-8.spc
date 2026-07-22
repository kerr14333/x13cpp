# EDGE CASE: series{ saveprecision = 8 } changes the /rdb save-file numeric
# format from the default (sp,e22.15) to (sp,e15.08). Gates the saveprecision
# fix (run_pre_model.cpp: svsize/svfmt are now derived from the parsed svprec
# per gtinpt.f:1185-1187, instead of being clobbered back to 15). The golden a1
# save file is written at 8 significant digits; the C++ must reproduce it
# byte-for-byte.
series{
  title         = "saveprecision gate"
  file          = "../data/airline.dat"
  start         = 1949.01
  period        = 12
  saveprecision = 8
  save          = (a1)
}
x11{ }
