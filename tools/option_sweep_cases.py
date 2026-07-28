HDR = 'series{ title="a" file="airline.dat" start=1949.01 period=12 }\n'
LOG = 'transform{ function=log }\n'
MDL = 'arima{ model=(0 1 1)(0 1 1) }\nestimate{ }\n'
X11 = 'x11{ }\n'


def spec(*blocks):
    return "".join(blocks)


BASE = spec(HDR, LOG, MDL, X11)
M3 = "x13run_m3"

C = []


def add(name, withtext, base=BASE, binary="x13run_x11"):
    C.append({"name": name, "base": base, "with": withtext, "bin": binary})


# ---- outlier{} : 12 args, none with a case in the C++ reader ---------------
OB = spec(HDR, LOG, MDL, 'outlier{ }\n', X11)
add("outlier types",        spec(HDR, LOG, MDL, 'outlier{ types=(ao) }\n', X11), OB, M3)
add("outlier method",       spec(HDR, LOG, MDL, 'outlier{ method=addall }\n', X11), OB, M3)
add("outlier critical",     spec(HDR, LOG, MDL, 'outlier{ critical=2.5 }\n', X11), OB, M3)
add("outlier lsrun",        spec(HDR, LOG, MDL, 'outlier{ critical=2.5 lsrun=3 }\n', X11), OB, M3)
add("outlier span",         spec(HDR, LOG, MDL, 'outlier{ critical=2.5 span=(1955.01,1958.12) }\n', X11), OB, M3)
add("outlier tcrate",       spec(HDR, LOG, MDL, 'outlier{ types=(tc) critical=2.5 tcrate=0.5 }\n', X11), OB, M3)
add("outlier almost",       spec(HDR, LOG, MDL, 'outlier{ critical=2.5 almost=0.5 }\n', X11), OB, M3)
add("outlier crit.alpha",   spec(HDR, LOG, MDL, 'outlier{ criticalalpha=0.10 }\n', X11), OB, M3)
add("outlier defaultcrit",  spec(HDR, LOG, MDL, 'outlier{ defaultcritical=yes }\n', X11), OB, M3)

# ---- estimate{} ------------------------------------------------------------
add("estimate outofsample", spec(HDR, LOG, 'arima{ model=(0 1 1)(0 1 1) }\nestimate{ outofsample=yes }\n', X11), BASE, M3)
add("estimate removeconst", spec(HDR, LOG, 'arima{ model=(0 1 1)(0 1 1) }\nestimate{ removeconstant=yes }\n', X11), BASE, M3)
add("estimate exact",       spec(HDR, LOG, 'arima{ model=(0 1 1)(0 1 1) }\nestimate{ exact=none }\n', X11), BASE, M3)
add("estimate maxiter",     spec(HDR, LOG, 'arima{ model=(0 1 1)(0 1 1) }\nestimate{ maxiter=2 }\n', X11), BASE, M3)
add("estimate tol",         spec(HDR, LOG, 'arima{ model=(0 1 1)(0 1 1) }\nestimate{ tol=1e-2 }\n', X11), BASE, M3)

# ---- regression{} ----------------------------------------------------------
RB = spec(HDR, LOG, 'regression{ variables=(td easter[8]) }\n', MDL, X11)
add("regression noapply",   spec(HDR, LOG, 'regression{ variables=(td easter[8]) noapply=(td) }\n', MDL, X11), RB)
add("regression tcrate",    spec(HDR, LOG, 'regression{ variables=(tc1955.jan) tcrate=0.5 }\n', MDL, X11), spec(HDR, LOG, 'regression{ variables=(tc1955.jan) }\n', MDL, X11))
add("regression eastermeans", spec(HDR, LOG, 'regression{ variables=(easter[8]) eastermeans=no }\n', MDL, X11), spec(HDR, LOG, 'regression{ variables=(easter[8]) }\n', MDL, X11))
add("regression centeruser", spec(HDR, LOG, 'regression{ variables=(td) centeruser=mean }\n', MDL, X11), spec(HDR, LOG, 'regression{ variables=(td) }\n', MDL, X11))
add("regression aicdiff",   spec(HDR, LOG, 'regression{ aictest=(td) aicdiff=(-5) }\n', MDL, X11), spec(HDR, LOG, 'regression{ aictest=(td) }\n', MDL, X11), M3)
add("regression pvaictest", spec(HDR, LOG, 'regression{ aictest=(td) pvaictest=0.01 }\n', MDL, X11), spec(HDR, LOG, 'regression{ aictest=(td) }\n', MDL, X11), M3)
add("regression testalleaster", spec(HDR, LOG, 'regression{ aictest=(easter) testalleaster=yes }\n', MDL, X11), spec(HDR, LOG, 'regression{ aictest=(easter) }\n', MDL, X11), M3)
add("regression chi2test",  spec(HDR, LOG, 'regression{ variables=(td) chi2test=yes }\n', MDL, X11), spec(HDR, LOG, 'regression{ variables=(td) }\n', MDL, X11), M3)
add("regression tlimit",    spec(HDR, LOG, 'regression{ aictest=(td) tlimit=0.5 }\n', MDL, X11), spec(HDR, LOG, 'regression{ aictest=(td) }\n', MDL, X11), M3)

# ---- transform{} -----------------------------------------------------------
add("transform aicdiff",    spec(HDR, 'transform{ function=auto aicdiff=-5 }\n', MDL, X11), spec(HDR, 'transform{ function=auto }\n', MDL, X11), M3)
add("transform power",      spec(HDR, 'transform{ power=0.5 }\n', MDL, X11), spec(HDR, LOG, MDL, X11), M3)
add("transform adjustreg",  spec(HDR, LOG, 'regression{ variables=(td) }\n', MDL, 'transform{ function=log adjustreg=(td) }\n', X11), RB)

# ---- forecast{} ------------------------------------------------------------
add("forecast maxlead",     spec(HDR, LOG, MDL, 'forecast{ maxlead=24 }\n', X11), BASE)
add("forecast probability", spec(HDR, LOG, MDL, 'forecast{ probability=0.80 }\n', X11), BASE)
add("forecast exclude",     spec(HDR, LOG, MDL, 'forecast{ exclude=12 }\n', X11), BASE)
add("forecast lognormal",   spec(HDR, LOG, MDL, 'forecast{ lognormal=yes }\n', X11), BASE)

# ---- identify{} : whole block via gt_generic -------------------------------
add("identify diff/sdiff",  spec(HDR, LOG, 'identify{ diff=(0,1) sdiff=(0,1) maxlag=24 }\n'),
    spec(HDR, LOG, 'identify{ }\n'), M3)

# ---- spectrum{} leftovers --------------------------------------------------
add("spectrum start",       spec(HDR, LOG, MDL, 'spectrum{ start=1955.01 }\n', X11), BASE)
add("spectrum siglevel",    spec(HDR, LOG, MDL, 'spectrum{ siglevel=8 }\n', X11), BASE)
add("spectrum peakwidth",   spec(HDR, LOG, MDL, 'spectrum{ peakwidth=2 }\n', X11), BASE)
add("spectrum altfreq",     spec(HDR, LOG, MDL, 'spectrum{ altfreq=yes }\n', X11), BASE)
add("spectrum tukey120",    spec(HDR, LOG, MDL, 'spectrum{ tukey120=yes }\n', X11), BASE)
add("spectrum qcheck",      spec(HDR, LOG, MDL, 'spectrum{ qcheck=yes }\n', X11), BASE)
add("spectrum showseasfreq", spec(HDR, LOG, MDL, 'spectrum{ showseasonalfreq=yes }\n', X11), BASE)
add("spectrum saveallfreq", spec(HDR, LOG, MDL, 'spectrum{ saveallfreq=yes }\n', X11), BASE)
add("spectrum localpeak",   spec(HDR, LOG, MDL, 'spectrum{ localpeak=yes }\n', X11), BASE)
add("spectrum maxar",       spec(HDR, LOG, MDL, 'spectrum{ maxar=10 }\n', X11), BASE)

# ---- x11{} leftovers -------------------------------------------------------
add("x11 taper",            spec(HDR, LOG, MDL, 'x11{ taper=10 }\n'), BASE)
add("x11 trendic",          spec(HDR, LOG, MDL, 'x11{ trendic=2.0 }\n'), BASE)
add("x11 calendarsigma",    spec(HDR, LOG, MDL, 'x11{ calendarsigma=all }\n'), BASE)
add("x11 keepholiday",      spec(HDR, LOG, 'regression{ variables=(easter[8]) }\n', MDL, 'x11{ keepholiday=yes }\n'),
    spec(HDR, LOG, 'regression{ variables=(easter[8]) }\n', MDL, X11))
add("x11 final none",       spec(HDR, LOG, 'regression{ variables=(ao1955.jan) }\n', MDL, 'x11{ final=(none) }\n'),
    spec(HDR, LOG, 'regression{ variables=(ao1955.jan) }\n', MDL, X11))

CASES = C
