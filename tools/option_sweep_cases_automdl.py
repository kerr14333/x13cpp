SER = '''series{ title="a" file="airline.dat" start=1949.01 period=12 }
transform{ function=log }
'''
TAIL = 'x11{ }\n'


def amd(extra):
    return SER + "automdl{ " + extra + " }\n" + TAIL


BASE = amd("")

CASES = [
    {"name": "automdl maxorder",        "base": BASE, "bin": "x13run_m3", "with": amd("maxorder=(1,1)")},
    {"name": "automdl maxdiff",         "base": BASE, "bin": "x13run_m3", "with": amd("maxdiff=(1,1)")},
    {"name": "automdl diff",            "base": BASE, "bin": "x13run_m3", "with": amd("diff=(1,1)")},
    {"name": "automdl ub1",             "base": BASE, "bin": "x13run_m3", "with": amd("ub1=0.95")},
    {"name": "automdl ub2",             "base": BASE, "bin": "x13run_m3", "with": amd("ub2=0.80")},
    {"name": "automdl cancel",          "base": BASE, "bin": "x13run_m3", "with": amd("cancel=0.05")},
    {"name": "automdl balanced",        "base": BASE, "bin": "x13run_m3", "with": amd("balanced=yes")},
    {"name": "automdl exactdiff",       "base": BASE, "bin": "x13run_m3", "with": amd("exactdiff=no")},
    {"name": "automdl hrinitial",       "base": BASE, "bin": "x13run_m3", "with": amd("hrinitial=yes")},
    {"name": "automdl armalimit",       "base": BASE, "bin": "x13run_m3", "with": amd("armalimit=0.5")},
    {"name": "automdl reducecv",        "base": BASE, "bin": "x13run_m3", "with": amd("reducecv=0.25")},
    {"name": "automdl ljungboxlimit",   "base": BASE, "bin": "x13run_m3", "with": amd("ljungboxlimit=0.99")},
    {"name": "automdl urfinal",         "base": BASE, "bin": "x13run_m3", "with": amd("urfinal=1.10")},
    {"name": "automdl checkmu",         "base": BASE, "bin": "x13run_m3", "with": amd("checkmu=no")},
    {"name": "automdl mixed",           "base": BASE, "bin": "x13run_m3", "with": amd("mixed=no")},
    {"name": "automdl fcstlim",         "base": BASE, "bin": "x13run_m3", "with": amd("fcstlim=10")},
    {"name": "automdl rejectfcst",      "base": BASE, "bin": "x13run_m3", "with": amd("rejectfcst=yes")},
    {"name": "automdl seasonaloverdiff","base": BASE, "bin": "x13run_m3", "with": amd("seasonaloverdiff=0.90")},
    {"name": "automdl noautooutlier",   "base": BASE, "bin": "x13run_m3", "with": amd("noautooutlier=yes")},
    {"name": "automdl print",           "base": BASE, "bin": "x13run_m3", "with": amd("print=none")},
]
