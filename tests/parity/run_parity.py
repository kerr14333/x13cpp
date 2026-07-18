#!/usr/bin/env python
"""Parity harness: run an engine over the spec corpus and diff vs goldens.

Walks ``tests/corpus/**/*.spc``, runs an :class:`Engine` on each spec to produce
an output bundle, and compares that bundle against the stored golden bundle with
``x13compare``. Two engines exist:

* :class:`OracleEngine` -- runs the Fortran oracle binary via
  ``oracle/run_oracle.py``. Used both to *bless* goldens (``--update``) and, in
  self-calibration mode, to diff two oracle builds (e.g. -O0 vs -O2).
* :class:`CppEngine` -- placeholder for the C++ port; raises
  ``NotImplementedError`` until the engine exists.

Typical use::

    # bless goldens from the oracle
    python run_parity.py --binary ../../oracle/fortran/x13as_ascii_O0.exe --update

    # later: check an engine against goldens
    python run_parity.py --binary <bin> --engine oracle
"""

from __future__ import annotations

import argparse
import fnmatch
import importlib.util
import os
import sys
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from typing import List, Optional

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_ORACLE_DIR = os.path.join(_REPO, "oracle")
_COMPARE_DIR = os.path.join(_REPO, "tests", "compare")

# Make oracle/run_oracle.py and the x13compare package importable.
for _p in (_ORACLE_DIR, _COMPARE_DIR):
    if _p not in sys.path:
        sys.path.insert(0, _p)


def _load_run_oracle():
    """Import oracle/run_oracle.py as a module (path may contain no package)."""
    path = os.path.join(_ORACLE_DIR, "run_oracle.py")
    spec = importlib.util.spec_from_file_location("run_oracle", path)
    mod = importlib.util.module_from_spec(spec)
    assert spec and spec.loader
    # Register before exec so @dataclass can resolve the module namespace.
    sys.modules["run_oracle"] = mod
    spec.loader.exec_module(mod)
    return mod


run_oracle_mod = _load_run_oracle()
from x13compare.compare import compare_bundles, load_tolerances  # noqa: E402


DEFAULT_CORPUS = os.path.join(_REPO, "tests", "corpus")
DEFAULT_GOLDEN = os.path.join(_REPO, "tests", "golden")
DEFAULT_RUNS = os.path.join(_REPO, "tests", "parity", "_runs")
DEFAULT_TOL = os.path.join(_COMPARE_DIR, "x13compare", "tolerances.yaml")


# --------------------------------------------------------------------------- #
# Engine abstraction
# --------------------------------------------------------------------------- #
class Engine(ABC):
    name = "engine"

    @abstractmethod
    def run(self, spec_path: str, outdir: str, flags: List[str]) -> str:
        """Run the engine on *spec_path*, producing a bundle in *outdir*.
        Returns the bundle directory path."""
        raise NotImplementedError


class OracleEngine(Engine):
    name = "oracle"

    def __init__(self, binary: str, timeout: Optional[float] = None):
        self.binary = binary
        self.timeout = timeout

    def run(self, spec_path: str, outdir: str, flags: List[str]) -> str:
        result = run_oracle_mod.run_oracle(
            binary=self.binary,
            spec=spec_path,
            outdir=outdir,
            flags=flags,
            timeout=self.timeout,
        )
        return result.bundle_dir


class CppEngine(Engine):
    name = "cpp"

    def __init__(self, binary: Optional[str] = None):
        self.binary = binary

    def run(self, spec_path: str, outdir: str, flags: List[str]) -> str:
        raise NotImplementedError(
            "CppEngine is not implemented yet; the C++ port is a later phase."
        )


def make_engine(kind: str, binary: Optional[str], timeout=None) -> Engine:
    if kind == "oracle":
        if not binary:
            raise SystemExit("--binary is required for the oracle engine")
        return OracleEngine(binary, timeout=timeout)
    if kind == "cpp":
        return CppEngine(binary)
    raise SystemExit(f"unknown engine: {kind}")


# --------------------------------------------------------------------------- #
# Corpus walking + parity run
# --------------------------------------------------------------------------- #
@dataclass
class SpecCase:
    spec_path: str
    rel_id: str          # corpus-relative id without extension, e.g. "x11/sales"


@dataclass
class ParityOutcome:
    case: SpecCase
    status: str          # pass | fail | blessed | error | skipped
    note: str = ""
    n_compared: int = 0
    n_mismatch: int = 0


@dataclass
class ParitySummary:
    outcomes: List[ParityOutcome] = field(default_factory=list)

    @property
    def failed(self) -> bool:
        return any(o.status in ("fail", "error") for o in self.outcomes)

    @property
    def exit_code(self) -> int:
        return 1 if self.failed else 0

    def text(self) -> str:
        lines = []
        for o in self.outcomes:
            tag = o.status.upper()
            line = f"  [{tag}] {o.case.rel_id}"
            if o.n_compared or o.n_mismatch:
                line += f"  ({o.n_compared} compared, {o.n_mismatch} diffs)"
            if o.note:
                line += f"  -- {o.note}"
            lines.append(line)
        n_pass = sum(o.status in ("pass", "blessed") for o in self.outcomes)
        lines.append("")
        lines.append(f"{n_pass}/{len(self.outcomes)} cases OK; "
                     f"result: {'FAIL' if self.failed else 'PASS'}")
        return "\n".join(lines)

    def to_junit_xml(self) -> str:
        from xml.sax.saxutils import escape, quoteattr
        n = max(len(self.outcomes), 1)
        nf = sum(o.status in ("fail", "error") for o in self.outcomes)
        out = ['<?xml version="1.0" encoding="UTF-8"?>',
               f'<testsuite name="x13parity" tests="{n}" failures="{nf}">']
        for o in self.outcomes:
            out.append(f'  <testcase classname="x13parity" '
                       f'name={quoteattr(o.case.rel_id)}>')
            if o.status in ("fail", "error"):
                out.append(f'    <failure message={quoteattr(o.note or o.status)}>'
                           f'{escape(o.note)}</failure>')
            elif o.status == "skipped":
                out.append(f'    <skipped message={quoteattr(o.note)}/>')
            out.append('  </testcase>')
        out.append('</testsuite>')
        return "\n".join(out)


def discover_specs(corpus_dir: str, pattern: str = "*") -> List[SpecCase]:
    cases: List[SpecCase] = []
    for root, _dirs, files in os.walk(corpus_dir):
        for fn in sorted(files):
            if not fn.lower().endswith(".spc"):
                continue
            full = os.path.join(root, fn)
            rel = os.path.relpath(os.path.splitext(full)[0], corpus_dir)
            rel_id = rel.replace(os.sep, "/")
            if not fnmatch.fnmatch(rel_id, pattern):
                continue
            cases.append(SpecCase(spec_path=full, rel_id=rel_id))
    return cases


def run_parity(
    engine: Engine,
    corpus_dir: str = DEFAULT_CORPUS,
    golden_dir: str = DEFAULT_GOLDEN,
    runs_dir: str = DEFAULT_RUNS,
    flags: Optional[List[str]] = None,
    tolerances_path: Optional[str] = DEFAULT_TOL,
    update: bool = False,
    pattern: str = "*",
) -> ParitySummary:
    flags = flags if flags is not None else ["-s"]
    tol = load_tolerances(tolerances_path if tolerances_path and
                          os.path.exists(tolerances_path) else None)
    summary = ParitySummary()
    cases = discover_specs(corpus_dir, pattern)

    for case in cases:
        golden_bundle = os.path.join(golden_dir, case.rel_id)
        try:
            if update:
                engine.run(case.spec_path, golden_bundle, flags)
                summary.outcomes.append(
                    ParityOutcome(case, "blessed",
                                  note=f"golden written to {golden_bundle}"))
                continue

            if not os.path.isdir(golden_bundle):
                summary.outcomes.append(
                    ParityOutcome(case, "skipped",
                                  note="no golden bundle (run with --update)"))
                continue

            run_bundle = os.path.join(runs_dir, engine.name, case.rel_id)
            engine.run(case.spec_path, run_bundle, flags)
            report = compare_bundles(golden_bundle, run_bundle, tol)
            n_cmp = report.total_compared
            n_mis = len(report.mismatches)
            if report.passed:
                summary.outcomes.append(
                    ParityOutcome(case, "pass", n_compared=n_cmp))
            else:
                note = report.to_text_summary(max_per_file=5)
                summary.outcomes.append(
                    ParityOutcome(case, "fail", note=note,
                                  n_compared=n_cmp, n_mismatch=n_mis))
        except NotImplementedError as exc:
            summary.outcomes.append(ParityOutcome(case, "skipped", note=str(exc)))
        except Exception as exc:  # pragma: no cover - defensive
            summary.outcomes.append(
                ParityOutcome(case, "error", note=f"{type(exc).__name__}: {exc}"))

    return summary


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="run_parity.py",
        description="Run an engine over the corpus and diff vs golden bundles.",
    )
    p.add_argument("--binary", default=None, help="oracle binary path")
    p.add_argument("--engine", default="oracle", choices=["oracle", "cpp"])
    p.add_argument("--corpus", default=DEFAULT_CORPUS)
    p.add_argument("--golden", default=DEFAULT_GOLDEN)
    p.add_argument("--runs", default=DEFAULT_RUNS)
    p.add_argument("--tolerances", default=DEFAULT_TOL)
    p.add_argument("--flags", default="-s", help='engine flags as one string')
    p.add_argument("--filter", default="*", help="glob over corpus-relative ids")
    p.add_argument("--update", action="store_true",
                   help="bless: write goldens instead of comparing")
    p.add_argument("--timeout", type=float, default=None)
    p.add_argument("--junit", default=None)
    return p


def main(argv=None) -> int:
    import shlex
    args = build_parser().parse_args(argv)
    engine = make_engine(args.engine, args.binary, timeout=args.timeout)
    summary = run_parity(
        engine=engine,
        corpus_dir=args.corpus,
        golden_dir=args.golden,
        runs_dir=args.runs,
        flags=shlex.split(args.flags),
        tolerances_path=args.tolerances,
        update=args.update,
        pattern=args.filter,
    )
    print(summary.text())
    if args.junit:
        with open(args.junit, "w", encoding="utf-8") as fh:
            fh.write(summary.to_junit_xml())
    return summary.exit_code


if __name__ == "__main__":
    sys.exit(main())
