# BLS CES seasonal-adjustment spec for Logging (CEU1011330001, All Employees NSA).
# Sourced from https://www.bls.gov/web/empsit/ces.spec.ae.zip (ces.spec.ae/).
# Only the FILE paths were rewritten to repo-relative; the model/regression/
# outlier/x11 settings are BLS's verbatim.
SERIES{
TITLE = "Logging"
START = 2016.01
PERIOD = 12
SAVE = (A1 B1)
PRINT = BRIEF
NAME = '1011330000 - AE'
FILE = "data/AE1011330000.dat"}
TRANSFORM{FUNCTION = LOG

}
REGRESSION{
USER = (dum1 dum2 dum3 dum4 dum5 dum6 dum7 dum8 dum9 dum10 dum11)
START = 1986.01
USERTYPE = TD
FILE = "data/FDUM8606.dat"
SAVE = (TD AO LS TC)
}
ARIMA{
MODEL = (0 1 0)(0 1 1)
}
ESTIMATE{
MAXITER = 3000
}
FORECAST{
MAXLEAD = 24
}
OUTLIER{
CRITICAL = 10.5
TYPES = AO
}
X11{
SEASONALMA = (s3x5)
MODE = MULT
PRINT = (BRIEF)
SAVE = (D10 D11 D16)
APPENDFCST = YES
FINAL = USER
SAVELOG = (Q Q2 M7 FB1 FD8 MSF)
}
