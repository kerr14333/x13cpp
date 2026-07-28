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


SERIES = {
    # BLS CES, not seasonally adjusted -- tests/corpus/data/ces_PROVENANCE.md
    "ces_amuse":   _hdr("ces_amuse", "1990.01", 12, "log"),
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
ARGS = [
    ("maxorder",         "maxorder=(1,1)"),
    ("maxdiff",          "maxdiff=(1,1)"),
    ("diff",             "diff=(1,1)"),
    ("ub1",              "ub1=1.02"),
    ("ub2",              "ub2=0.80"),
    ("cancel",           "cancel=0.05"),
    ("balanced",         "balanced=yes"),
    ("exactdiff",        "exactdiff=no"),
    ("hrinitial",        "hrinitial=yes"),
    ("armalimit",        "armalimit=0.5"),
    ("percentrse",       "percentrse=2.0"),
    ("reducecv",         "reducecv=0.25"),
    ("ljungboxlimit",    "ljungboxlimit=0.99"),
    ("acceptdefault",    "acceptdefault=yes"),
    ("noautooutlier",    "noautooutlier=tramo"),
    ("urfinal",          "urfinal=1.10"),
    ("firstar",          "firstar=2"),
    ("checkmu",          "checkmu=no"),
    ("mixed",            "mixed=no"),
    ("rejectfcst",       "rejectfcst=yes"),
    ("fcstlim",          "fcstlim=10"),
    ("seasonaloverdiff", "seasonaloverdiff=no"),
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
