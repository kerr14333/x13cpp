"""Compare two X-13 "golden bundles" numerically and produce a report.

A *bundle* is a directory produced by ``oracle/run_oracle.py``: it holds the
run's ``.out`` / ``.err`` / ``.log`` / ``.udg`` files, individual ``save``
tables, and a ``manifest.json``. :func:`compare_bundles` walks the files common
to both bundles, parses each by type, and compares:

* ``.out``   -> table cells (numeric, per-table tolerance) + text lines
  (whitespace-normalised exact).
* save files -> ``{period: value}`` numeric, per-table tolerance.
* ``.udg``   -> numeric values with tolerance, strings exact.
* ``.err`` / ``.log`` -> whitespace-normalised text lines.

Numeric closeness uses the standard ``abs(a-b) <= atol + rtol*abs(b)`` test,
with rtol default ``1e-8`` / atol default ``1e-12`` and per-table overrides from
a tolerance classification file (see :func:`load_tolerances` and
``tolerances.yaml``).
"""

from __future__ import annotations

import json
import math
import os
import re
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

from . import parse_out, parse_save, parse_udg

DEFAULT_RTOL = 1e-8
DEFAULT_ATOL = 1e-12

# File extensions handled specially; everything else that isn't an obvious
# input/manifest file is treated as a numeric save table.
_TEXT_EXTS = {".err", ".log"}
_SKIP_EXTS = {".spc", ".json", ".dta", ".dat", ".xml"}
_SKIP_NAMES = {"manifest.json"}

_WS_RE = re.compile(r"\s+")


def _norm_ws(s: str) -> str:
    return _WS_RE.sub(" ", s.strip())


# Run timestamps appear in .out/.log headers ("Jul 18, 2026 14.25.37" or
# "Execution began/complete ... at ...") and differ between any two runs;
# they carry no parity information, so mask them before text comparison.
_TIMESTAMP_RE = re.compile(
    r"(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+\d{1,2},\s+\d{4}"
    r"(\s+\d{1,2}\.\d{2}\.\d{2})?")

# .udg bookkeeping keys that record the wall-clock run time, not results.
_UDG_IGNORE_KEYS = {"date", "time"}


def _mask_timestamps(s: str) -> str:
    return _TIMESTAMP_RE.sub("<TIMESTAMP>", s)


# --------------------------------------------------------------------------- #
# Tolerances
# --------------------------------------------------------------------------- #
@dataclass
class Tolerances:
    default_rtol: float = DEFAULT_RTOL
    default_atol: float = DEFAULT_ATOL
    # class name -> (rtol, atol)
    class_tol: Dict[str, Tuple[float, float]] = field(default_factory=dict)
    # base table id -> class name
    table_class: Dict[str, str] = field(default_factory=dict)

    def resolve(self, table_id: Optional[str]) -> Tuple[float, float, str]:
        """Return (rtol, atol, class_name) for a table id (or None)."""
        if table_id:
            base = _base_id(table_id)
            cls = self.table_class.get(base)
            if cls and cls in self.class_tol:
                rtol, atol = self.class_tol[cls]
                return rtol, atol, cls
        return self.default_rtol, self.default_atol, "default"


def _base_id(table_id: str) -> str:
    """Strip duplicate suffix (``d11#2`` -> ``d11``) and lowercase."""
    return table_id.split("#", 1)[0].strip().lower()


def load_tolerances(path: Optional[str] = None) -> Tolerances:
    """Load a tolerance classification file (YAML if available, else JSON).

    Schema::

        default: {rtol: 1.0e-8, atol: 1.0e-12}
        classes:
          optimizer_dependent:
            rtol: 1.0e-6
            atol: 1.0e-9
            tables: [est, lks, rts, rsd, fct, bct, mdl]
    """
    tol = Tolerances()
    if path is None:
        return tol
    data = _load_config(path)
    default = data.get("default") or {}
    tol.default_rtol = float(default.get("rtol", DEFAULT_RTOL))
    tol.default_atol = float(default.get("atol", DEFAULT_ATOL))
    classes = data.get("classes") or {}
    for name, spec in classes.items():
        spec = spec or {}
        rtol = float(spec.get("rtol", tol.default_rtol))
        atol = float(spec.get("atol", tol.default_atol))
        tol.class_tol[name] = (rtol, atol)
        for t in spec.get("tables", []) or []:
            tol.table_class[str(t).strip().lower()] = name
    return tol


def _load_config(path: str) -> dict:
    with open(path, "r", encoding="utf-8") as fh:
        text = fh.read()
    if path.lower().endswith((".yaml", ".yml")):
        try:
            import yaml  # type: ignore
            return yaml.safe_load(text) or {}
        except Exception:
            # Fall through to JSON attempt if PyYAML unavailable / fails.
            pass
    return json.loads(text)


# --------------------------------------------------------------------------- #
# Report model
# --------------------------------------------------------------------------- #
@dataclass
class Mismatch:
    file: str
    section: str       # table id / "text" / udg / save label
    key: str           # cell/period/line identifier
    kind: str          # numeric | missing_a | missing_b | text | type
    a: object = None
    b: object = None
    detail: str = ""

    def __str__(self) -> str:
        loc = f"{self.file}:{self.section}:{self.key}"
        if self.kind == "numeric":
            return f"{loc} numeric a={self.a!r} b={self.b!r} {self.detail}"
        if self.kind in ("missing_a", "missing_b"):
            return f"{loc} {self.kind} ({self.detail})"
        return f"{loc} {self.kind} a={self.a!r} b={self.b!r} {self.detail}"


@dataclass
class FileResult:
    name: str
    compared: int = 0
    mismatches: List[Mismatch] = field(default_factory=list)
    status: str = "ok"          # ok | mismatch | only_a | only_b | error
    note: str = ""

    @property
    def passed(self) -> bool:
        return self.status == "ok" and not self.mismatches


@dataclass
class CompareReport:
    bundle_a: str
    bundle_b: str
    files: List[FileResult] = field(default_factory=list)

    @property
    def mismatches(self) -> List[Mismatch]:
        out: List[Mismatch] = []
        for f in self.files:
            out.extend(f.mismatches)
        return out

    @property
    def passed(self) -> bool:
        return all(f.passed for f in self.files)

    @property
    def exit_code(self) -> int:
        return 0 if self.passed else 1

    @property
    def total_compared(self) -> int:
        return sum(f.compared for f in self.files)

    # -- writers ----------------------------------------------------------- #
    def to_text_summary(self, max_per_file: int = 20) -> str:
        lines = [
            f"Bundle A: {self.bundle_a}",
            f"Bundle B: {self.bundle_b}",
            f"Result:   {'PASS' if self.passed else 'FAIL'}  "
            f"({self.total_compared} values compared, "
            f"{len(self.mismatches)} mismatches)",
            "",
        ]
        for f in self.files:
            tag = "PASS" if f.passed else f.status.upper()
            line = f"  [{tag}] {f.name}  ({f.compared} compared, {len(f.mismatches)} diffs)"
            if f.note:
                line += f"  -- {f.note}"
            lines.append(line)
            for mm in f.mismatches[:max_per_file]:
                lines.append(f"        {mm}")
            if len(f.mismatches) > max_per_file:
                lines.append(f"        ... {len(f.mismatches) - max_per_file} more")
        return "\n".join(lines)

    def to_junit_xml(self) -> str:
        from xml.sax.saxutils import escape, quoteattr

        n_tests = max(len(self.files), 1)
        n_failures = sum(0 if f.passed else 1 for f in self.files)
        out = [
            '<?xml version="1.0" encoding="UTF-8"?>',
            f'<testsuite name="x13compare" tests="{n_tests}" '
            f'failures="{n_failures}">',
        ]
        for f in self.files:
            out.append(
                f"  <testcase classname=\"x13compare\" "
                f"name={quoteattr(f.name)}>"
            )
            if not f.passed:
                if f.status in ("only_a", "only_b", "error"):
                    msg = f.note or f.status
                    body = f.note
                else:
                    msg = f"{len(f.mismatches)} mismatch(es)"
                    body = "\n".join(str(m) for m in f.mismatches[:200])
                out.append(
                    f"    <failure message={quoteattr(msg)}>"
                    f"{escape(body)}</failure>"
                )
            out.append("  </testcase>")
        out.append("</testsuite>")
        return "\n".join(out)

    def write_junit(self, path: str) -> None:
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(self.to_junit_xml())

    def write_text_summary(self, path: str) -> None:
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(self.to_text_summary())


# --------------------------------------------------------------------------- #
# Numeric comparison
# --------------------------------------------------------------------------- #
def numbers_close(a: float, b: float, rtol: float, atol: float) -> bool:
    if a == b:
        return True
    if math.isnan(a) and math.isnan(b):
        return True
    if math.isinf(a) or math.isinf(b):
        return a == b
    return abs(a - b) <= atol + rtol * abs(b)


# --------------------------------------------------------------------------- #
# Bundle comparison
# --------------------------------------------------------------------------- #
def _list_bundle_files(bundle_dir: str) -> Dict[str, str]:
    """Return {lower-basename: fullpath} for comparable files in a bundle."""
    out: Dict[str, str] = {}
    for name in os.listdir(bundle_dir):
        full = os.path.join(bundle_dir, name)
        if not os.path.isfile(full):
            continue
        if name.lower() in _SKIP_NAMES:
            continue
        ext = os.path.splitext(name)[1].lower()
        if ext in _SKIP_EXTS:
            continue
        out[name.lower()] = full
    return out


def _classify(name: str) -> str:
    ext = os.path.splitext(name)[1].lower()
    if ext == ".out":
        return "out"
    if ext == ".udg":
        return "udg"
    if ext in _TEXT_EXTS:
        return "text"
    return "save"


def compare_bundles(
    bundle_a: str,
    bundle_b: str,
    tolerances: Optional[Tolerances] = None,
) -> CompareReport:
    tol = tolerances or Tolerances()
    report = CompareReport(bundle_a=bundle_a, bundle_b=bundle_b)

    files_a = _list_bundle_files(bundle_a)
    files_b = _list_bundle_files(bundle_b)
    all_names = sorted(set(files_a) | set(files_b))

    for name in all_names:
        fr = FileResult(name=name)
        if name not in files_a:
            fr.status = "only_b"
            fr.note = "present only in bundle B"
            report.files.append(fr)
            continue
        if name not in files_b:
            fr.status = "only_a"
            fr.note = "present only in bundle A"
            report.files.append(fr)
            continue

        kind = _classify(name)
        try:
            text_a = _read(files_a[name])
            text_b = _read(files_b[name])
            if kind == "out":
                _compare_out(fr, text_a, text_b, tol)
            elif kind == "udg":
                _compare_udg(fr, text_a, text_b, tol)
            elif kind == "text":
                _compare_text(fr, text_a, text_b)
            else:
                _compare_save(fr, name, text_a, text_b, tol)
        except Exception as exc:  # pragma: no cover - defensive
            fr.status = "error"
            fr.note = f"{type(exc).__name__}: {exc}"

        if fr.mismatches and fr.status == "ok":
            fr.status = "mismatch"
        report.files.append(fr)

    return report


def _read(path: str) -> str:
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        return fh.read()


def _save_ext_id(name: str) -> str:
    """Table id used for tolerance lookup of a save file (its extension)."""
    ext = os.path.splitext(name)[1].lstrip(".").lower()
    return ext


def _compare_out(fr: FileResult, ta: str, tb: str, tol: Tolerances) -> None:
    pa = parse_out.parse_out(ta)
    pb = parse_out.parse_out(tb)

    for tid in sorted(set(pa.tables) | set(pb.tables)):
        ta_tbl = pa.tables.get(tid)
        tb_tbl = pb.tables.get(tid)
        if ta_tbl is None:
            fr.mismatches.append(Mismatch(fr.name, tid, "-", "missing_a",
                                          detail="table absent in A"))
            continue
        if tb_tbl is None:
            fr.mismatches.append(Mismatch(fr.name, tid, "-", "missing_b",
                                          detail="table absent in B"))
            continue
        rtol, atol, _cls = tol.resolve(tid)
        keys = set(ta_tbl.cells) | set(tb_tbl.cells)
        for key in sorted(keys, key=lambda k: (str(k[0]), str(k[1]))):
            va = ta_tbl.cells.get(key)
            vb = tb_tbl.cells.get(key)
            cellkey = f"{key[0]}/{key[1]}"
            if va is None:
                fr.mismatches.append(Mismatch(fr.name, tid, cellkey, "missing_a"))
                continue
            if vb is None:
                fr.mismatches.append(Mismatch(fr.name, tid, cellkey, "missing_b"))
                continue
            fr.compared += 1
            if not numbers_close(va, vb, rtol, atol):
                fr.mismatches.append(Mismatch(
                    fr.name, tid, cellkey, "numeric", va, vb,
                    detail=f"|d|={abs(va - vb):.3e} (rtol={rtol:g},atol={atol:g})"))

    # Text lines (whitespace-normalised exact, order-sensitive).
    _compare_text_lines(fr, pa.text_lines, pb.text_lines, section="text")


def _compare_save(fr: FileResult, name: str, ta: str, tb: str,
                  tol: Tolerances) -> None:
    pa = parse_save.parse_save(ta)
    pb = parse_save.parse_save(tb)
    rtol, atol, _cls = tol.resolve(_save_ext_id(name))
    keys = set(pa.rows) | set(pb.rows)
    for period in sorted(keys):
        ra = pa.rows.get(period)
        rb = pb.rows.get(period)
        if ra is None:
            fr.mismatches.append(Mismatch(fr.name, pa.label or name, period, "missing_a"))
            continue
        if rb is None:
            fr.mismatches.append(Mismatch(fr.name, pb.label or name, period, "missing_b"))
            continue
        if len(ra) != len(rb):
            fr.mismatches.append(Mismatch(
                fr.name, name, period, "type", ra, rb,
                detail="differing column count"))
            continue
        for ci, (va, vb) in enumerate(zip(ra, rb)):
            fr.compared += 1
            if not numbers_close(va, vb, rtol, atol):
                col = period if len(ra) == 1 else f"{period}[{ci}]"
                fr.mismatches.append(Mismatch(
                    fr.name, name, col, "numeric", va, vb,
                    detail=f"|d|={abs(va - vb):.3e} (rtol={rtol:g},atol={atol:g})"))


def _compare_udg(fr: FileResult, ta: str, tb: str, tol: Tolerances) -> None:
    pa = parse_udg.parse_udg(ta)
    pb = parse_udg.parse_udg(tb)
    rtol, atol = tol.default_rtol, tol.default_atol
    for key in sorted(set(pa) | set(pb)):
        if key in _UDG_IGNORE_KEYS:
            continue
        va = pa.get(key)
        vb = pb.get(key)
        if key not in pa:
            fr.mismatches.append(Mismatch(fr.name, "udg", key, "missing_a"))
            continue
        if key not in pb:
            fr.mismatches.append(Mismatch(fr.name, "udg", key, "missing_b"))
            continue
        fr.compared += 1
        _compare_value(fr, "udg", key, va, vb, rtol, atol)


def _compare_value(fr, section, key, va, vb, rtol, atol) -> None:
    if isinstance(va, float) and isinstance(vb, float):
        if not numbers_close(va, vb, rtol, atol):
            fr.mismatches.append(Mismatch(
                fr.name, section, key, "numeric", va, vb,
                detail=f"|d|={abs(va - vb):.3e}"))
        return
    if isinstance(va, list) and isinstance(vb, list):
        if len(va) != len(vb):
            fr.mismatches.append(Mismatch(fr.name, section, key, "type", va, vb,
                                          detail="differing length"))
            return
        for i, (x, y) in enumerate(zip(va, vb)):
            if not numbers_close(x, y, rtol, atol):
                fr.mismatches.append(Mismatch(
                    fr.name, section, f"{key}[{i}]", "numeric", x, y,
                    detail=f"|d|={abs(x - y):.3e}"))
        return
    # Mixed types or strings -> normalised-string exact.
    if _norm_ws(str(va)) != _norm_ws(str(vb)):
        fr.mismatches.append(Mismatch(fr.name, section, key, "text", va, vb))


def _compare_text(fr: FileResult, ta: str, tb: str) -> None:
    la = [_norm_ws(x) for x in ta.splitlines() if x.strip()]
    lb = [_norm_ws(x) for x in tb.splitlines() if x.strip()]
    _compare_text_lines(fr, la, lb, section="text", pre_normalised=True)


def _compare_text_lines(fr, la, lb, section, pre_normalised=False) -> None:
    if not pre_normalised:
        la = [_norm_ws(x) for x in la if x.strip()]
        lb = [_norm_ws(x) for x in lb if x.strip()]
    la = [_mask_timestamps(x) for x in la]
    lb = [_mask_timestamps(x) for x in lb]
    fr.compared += min(len(la), len(lb))
    n = max(len(la), len(lb))
    for i in range(n):
        xa = la[i] if i < len(la) else None
        xb = lb[i] if i < len(lb) else None
        if xa is None:
            fr.mismatches.append(Mismatch(fr.name, section, f"line{i}", "missing_a", detail=repr(xb)))
        elif xb is None:
            fr.mismatches.append(Mismatch(fr.name, section, f"line{i}", "missing_b", detail=repr(xa)))
        elif xa != xb:
            fr.mismatches.append(Mismatch(fr.name, section, f"line{i}", "text", xa, xb))
