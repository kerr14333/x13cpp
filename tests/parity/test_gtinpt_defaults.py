"""gtinpt.f's default block, re-derived, against what the port actually writes.

The defect class this exists for has cost this port four separate increments and
always looks the same: `gtinpt.f` initialises a COMMON flag to a NON-ZERO value
before any spec is read, the port assigns that flag only inside the arm that
parses the option, and so every run that does not spell the option out gets the
struct's zero-init instead of the Census default. Nothing refuses, nothing is
walled, the guards LOOK ported, and the run returns `OUTCOME: OK` with wrong
numbers.

Found this way so far:

  Lindot   composite{indoutlier=}  defaults YES; four agr3 guards were dead, and
           the miss was invisible until a component carried an outlier because
           every consumer is a conjunction with Lindls/Lindao (entry 95).
  Aicstk   the day of month a STOCK series is measured on, gtinpt.f:291 = 31.
           Never written; automd.cpp additionally reset it to 0, a value the
           Fortran assigns nowhere. It is not a label parameter -- addtd.f
           selects the stock-TD variables and builds their values for that day,
           so `series{type=stock}` + `aictest=(td)` ran the entire test at day 0
           (entry 101).

BOTH SIDES ARE DERIVED, so this cannot go stale the way a hand-written list
does: the left side is parsed out of the vendored `gtinpt.f`, the right side out
of the C++ tree. A default deleted from either side leaves the comparison.

The rule, stated so it has a low false-positive rate rather than a low false-
negative one:

    if the oracle gives a field a NON-ZERO default,
    and the port declares that field,
    and something OUTSIDE core/src/specparse READS it,
    then the port must WRITE it somewhere with a non-zero value.

"Somewhere" is deliberately loose. Writing the default at the head of the owning
reader instead of at spec-parse start is a real and deliberate deviation in this
port -- gt_seats, gt_force, gt_history and gt_slidingspans all do it, on the
argument that nothing reads those COMMONs before the reader runs -- and this
test is not the place to relitigate it. What it catches is the case where the
Census default reaches NO write at all, or reaches only a write of zero.
"""
import pathlib
import re

import pytest

ROOT = pathlib.Path(__file__).resolve().parents[2]
GTINPT_F = ROOT / "oracle" / "fortran" / "gtinpt.f"
SRC = ROOT / "core" / "src"

# The "Set the default values" block: from the comment banner at :128 to the
# spec-dispatch DO WHILE at :566. Bounded by markers rather than line numbers so
# that a Census release shifting the file does not silently shrink the input.
BLOCK_START = "c     Set the default values"
BLOCK_END = "      DO WHILE (T)"

# Values that a C++ struct's zero-init already gives you. A default in this set
# cannot be the defect above.
ZERO_RHS = {"F", "0", "ZERO", "0D0", "0.0D0", ".false.", "0.0", "' '"}

# Known exceptions, each with the reason it is not a defect. An entry here is a
# claim that has been checked; it is not a way to silence the test.
ALLOWED = {
    # Eick (gtinpt.f:297) defaults to DNOTST, and BOTH of its consumers --
    # estimate.cpp:789 and prlkhd.f:289 -- test `Eick > 0`. DNOTST is negative
    # and the zero-init is 0.0, so the two agree on every branch; gtestm.f:263
    # additionally rejects a parsed Eick <= 0. Inert by the shape of the
    # predicate, not by luck of the corpus.
    "eick",
    # Targsa / Targtr / Rfctlg are COUNT-BOUNDED arrays: gtinpt.f fills them
    # with NOTSET, but every read is bounded by the companion count the parser
    # writes alongside them (Ntarsa, Ntartr, Nfctlg -- run_history.cpp:294-295,
    # :328, :141-143), so a slot that was never parsed is never reached and the
    # NOTSET-vs-zero distinction has no consumer. Note this is a claim about the
    # BOUND, not about the array: if a read ever iterates to PTARGT/PFCLAG
    # instead of the count, the entry has to come back out.
    "targsa",
    "targtr",
    "rfctlg",
}

# Fortran name -> the port's field name, where they differ. Empty today; kept so
# a rename does not have to become an ALLOWED entry.
RENAMED: dict = {}


def _fortran_defaults():
    """Parse gtinpt.f's default block into {lowercased name: (line, rhs)}."""
    text = GTINPT_F.read_text(encoding="latin-1").split("\n")
    try:
        lo = next(i for i, l in enumerate(text) if l.startswith(BLOCK_START))
        hi = next(i for i, l in enumerate(text) if l.startswith(BLOCK_END))
    except StopIteration:  # pragma: no cover - guarded by the floor test below
        pytest.fail("gtinpt.f default block markers not found -- has the "
                    "vendored oracle changed? Fix the markers, do not widen them.")
    out = {}
    for off, raw in enumerate(text[lo:hi]):
        n = lo + off + 1
        s = raw.rstrip()
        # Fixed-form Fortran: a comment is a marker in COLUMN 1 only. Testing
        # `lstrip().startswith('C')` instead drops every `CALL` line, which is
        # how the first pass of this audit missed 51 array defaults.
        if not s or s[0] in "cC*!":
            continue
        body = s[6:] if len(s) > 6 else ""
        m = re.match(r"\s*([A-Za-z]\w*)\s*(\([^)]*\))?\s*=\s*(.+?)\s*$", body)
        if m:
            out.setdefault(m.group(1).lower(), (n, m.group(3)))
            continue
        m = re.match(r"\s*CALL\s+set(?:int|lg|dp|chr)\(\s*([^,]+?)\s*,"
                     r"\s*[^,]+\s*,\s*(\w+)", body, re.I)
        if m:
            out.setdefault(m.group(2).lower(), (n, m.group(1)))
            continue
    return out


def _cpp_sources():
    return {p: p.read_text(encoding="utf-8", errors="replace").split("\n")
            for p in SRC.rglob("*.cpp")}


def _cpp_headers():
    return {p: p.read_text(encoding="utf-8", errors="replace")
            for p in SRC.rglob("*.hpp")}


_DECL = r"\b(?:bool|int|double|char)\s[^;=]*\b{f}\b\s*[;,=]|\b{f}\b\s*;\s*//"


def _declared(headers, field):
    pat = re.compile(_DECL.format(f=re.escape(field)))
    return any(pat.search(t) for t in headers.values())


def _reads_outside_specparse(sources, field):
    """Mentions of `.field` that are not a plain assignment TO it."""
    mention = re.compile(r"\." + re.escape(field) + r"\b")
    assign = re.compile(r"\." + re.escape(field) + r"\b\s*(\([^)]*\))?\s*=[^=]")
    hits = []
    for p, ls in sources.items():
        if "specparse" in p.parts:
            continue
        for i, l in enumerate(ls, 1):
            if mention.search(l) and not assign.search(l):
                hits.append((p, i))
    return hits


_ZERO_CPP = re.compile(r"^\s*(?:0|0\.0|0\.0f|false|nullptr)\s*$")


def _nonzero_write(sources, field):
    """A write `.field = <expr>` anywhere, whose RHS is not literally zero."""
    assign = re.compile(r"\." + re.escape(field) + r"\b\s*(?:\([^)]*\))?\s*=\s*([^;]+);")
    setter = re.compile(r"(?:setint|setdp|setlg)\(\s*([^,]+),[^;]*\." + re.escape(field))
    for p, ls in sources.items():
        for l in ls:
            m = assign.search(l)
            if m and not _ZERO_CPP.match(m.group(1)):
                return (p, l.strip())
            m = setter.search(l)
            if m and not _ZERO_CPP.match(m.group(1).strip()):
                return (p, l.strip())
    return None


def test_default_block_parsed():
    """Floor assertion: the discovery must not silently shrink to nothing.

    A parametrisation that shrinks reports green, so every derived list in this
    suite carries a floor. 370 defaults / 164 of them non-zero were counted on
    2026-08-09 against x13as v1.1 b61; the floors sit below that so a genuine
    Census change is a review, not a failure.
    """
    defaults = _fortran_defaults()
    nonzero = {k: v for k, v in defaults.items() if v[1] not in ZERO_RHS}
    assert len(defaults) >= 340, f"only {len(defaults)} defaults parsed"
    assert len(nonzero) >= 150, f"only {len(nonzero)} non-zero defaults parsed"


def test_checker_can_fail():
    """The positive control -- a check that cannot fail loudly is not a check.

    `eick` is a REAL member of the offender set (non-zero default, read outside
    the parser, no write anywhere); it is suppressed only by its ALLOWED entry.
    Dropping that entry must bring it back, which proves the parse, the declared
    check, the read scan and the write scan are all live -- the failure mode
    this guards against is the whole pipeline quietly matching nothing and
    reporting green.
    """
    defaults = _fortran_defaults()
    sources = _cpp_sources()
    headers = _cpp_headers()
    assert "eick" in defaults and defaults["eick"][1] not in ZERO_RHS
    assert _declared(headers, "eick")
    assert _reads_outside_specparse(sources, "eick")
    assert _nonzero_write(sources, "eick") is None


def test_nonzero_defaults_reach_a_write():
    defaults = _fortran_defaults()
    sources = _cpp_sources()
    headers = _cpp_headers()

    offenders = []
    for name, (line, rhs) in sorted(defaults.items()):
        if rhs in ZERO_RHS:
            continue
        field = RENAMED.get(name, name)
        if field in ALLOWED or not _declared(headers, field):
            continue
        if not _reads_outside_specparse(sources, field):
            continue          # nothing consumes it yet; not yet a defect
        if _nonzero_write(sources, field) is None:
            offenders.append(f"  {field:9s} gtinpt.f:{line} = {rhs}")

    assert not offenders, (
        "gtinpt.f gives these a non-zero default, the engine READS them outside "
        "the parser, and no write in core/src ever gives them that value -- so "
        "they run on the struct's zero-init:\n" + "\n".join(offenders) +
        "\n\nThis is the Lindot/Aicstk class. Set the default (gtinpt.cpp, or "
        "the owning reader's head), or add an ALLOWED entry saying why the two "
        "values are indistinguishable to every consumer."
    )
