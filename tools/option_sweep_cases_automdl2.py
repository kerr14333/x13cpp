"""automdl{} option probe, round 2 -- on SEASONAL, near-tied series.

Round 1 (``option_sweep_cases_automdl.py``) probed airline and measured every
argument INERT.  That was a saturated precondition, not coverage: airline's
top two candidates are 0.014 BIC apart, and these knobs adjust unit-root and
cancellation thresholds by amounts smaller than that.

Series picked on two measured criteria at once (oracle, automdl{} + x11{}):

    series    qsori     M7      bic2-bic1   top two
    ukgas     176.5     0.210     0.001     (1 0 2)(0 1 0) / (1 0 2)(0 1 1)
    co2       495.3     0.053     0.009     (0 1 1)(0 1 1) / (1 1 1)(0 1 1)
    nottem    237.8     0.127     0.018     (1 0 0)(1 1 1) / (2 0 0)(1 1 1)
    airline   167.6     0.202     0.014     -- round 1's base

ukgas is the only corpus series that is both strongly seasonal and near-tied,
and its tie is in the SEASONAL MA -- exactly what cancel/ub2/urfinal/armalimit
move.  co2 is the most seasonal series in the corpus.

unrate/payems/expgs are deliberately EXCLUDED despite unrate's 0.001 gap: all
three have qsori ~= 0 and M7 > 1, i.e. no identifiable seasonality, so their
near-ties are between NONSEASONAL candidates and cannot exercise a seasonal
threshold.
"""

def _hdr(f, start, per, fn):
    return ('series{ title="a" file="%s.dat" start=%s period=%d }\n'
            'transform{ function=%s }\n' % (f, start, per, fn))


# NB `ces_amuse` is deliberately NOT here despite having the narrowest BIC gap
# (0.001) of any corpus series. Its BASELINE model already disagrees with the
# oracle (engine 4 ARMA terms, oracle 5), so every verdict taken from it reads
# DIFFERS regardless of the option and is noise. That mistake cost one wrong
# finding: `ljungboxlimit` was reported APPLIED on ces_amuse alone and is in
# fact blocked. **Verify baseline agreement before adding a probe series.**
SERIES = {
    # BLS CES, not seasonally adjusted -- tests/corpus/data/ces_PROVENANCE.md
    "ces_leis":    _hdr("ces_leis", "1990.01", 12, "log"),
    "ces_accfood": _hdr("ces_accfood", "1990.01", 12, "log"),
    "ukgas":       _hdr("ukgas", "1960.1", 4, "log"),
    "nottem":      _hdr("nottem", "1920.01", 12, "none"),
}

TAIL = "x11{ }\n"

# (argument, non-default value).
#
# gtauto.f's ARGDIC carries TWENTY-FOUR arguments, not the 20 round 1 probed:
# `percentrse`, `acceptdefault` and `firstar` were simply missing from the list
# (`savelog` is print surface and is excluded on purpose).  An argument absent
# from the probe list reads exactly like an argument that measured INERT.
#
# Three round-1 values were also REJECTED by the oracle, i.e. never tested:
# `ub1` must be > 1 ("Initial unit root limit must be greater than one"),
# `seasonaloverdiff` takes yes/no (gtauto.f:479) and `noautooutlier` takes
# same/tramo (NOTDIC at gtauto.f:48, reached via label 180).  A REJECTED probe
# is not a null result -- it is an untested argument.
# Values are chosen FAR from the default, not merely different from it. Round 2
# read `ljungboxlimit` as INERT probing 0.99 against a 0.95 default; at 0.5 it
# moves three of the four series. A probe value adjacent to the default tests
# nothing, and reports the same word ("INERT") as an argument that genuinely
# does nothing. Defaults are in gtinpt.f:221-241 / gtinpt.cpp:215-241.
ARGS = [
    ("maxorder",         "maxorder=(1,1)"),       # default (2,1)
    ("maxdiff",          "maxdiff=(1,0)"),        # default (2,1)
    ("diff",             "diff=(1,1)"),           # no default; fixes the orders
    ("ub1",              "ub1=1.50"),             # default 1/0.96 = 1.0417
    ("ub2",              "ub2=0.50"),             # default 0.88
    ("cancel",           "cancel=0.50"),          # default 0.1
    ("balanced",         "balanced=yes"),         # default no
    ("exactdiff",        "exactdiff=no"),         # default first
    ("hrinitial",        "hrinitial=yes"),        # default no
    ("armalimit",        "armalimit=3.0"),        # default 1.0
    ("percentrse",       "percentrse=10.0"),
    ("reducecv",         "reducecv=0.50"),        # default 0.14286
    ("ljungboxlimit",    "ljungboxlimit=0.5"),    # default 0.95
    ("acceptdefault",    "acceptdefault=yes"),    # default no
    ("noautooutlier",    "noautooutlier=tramo"),  # default same
    ("urfinal",          "urfinal=1.50"),         # default 1.05
    ("firstar",          "firstar=4"),            # default 2
    ("checkmu",          "checkmu=no"),           # default yes
    ("mixed",            "mixed=no"),             # default yes
    ("rejectfcst",       "rejectfcst=yes"),       # default no
    ("fcstlim",          "fcstlim=50"),           # default 15.0
    ("seasonaloverdiff", "seasonaloverdiff=yes"), # default no
    ("print",            "print=none"),
]

CASES = []
for ser, hdr in SERIES.items():
    base = hdr + "automdl{ }\n" + TAIL
    for name, extra in ARGS:
        CASES.append({
            "name": "%s automdl %s" % (ser, name),
            "base": base,
            "bin": "x13run_m3",
            "with": hdr + "automdl{ " + extra + " }\n" + TAIL,
        })
