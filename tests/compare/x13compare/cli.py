"""Command-line entry point: ``python -m x13compare A B [options]``."""

from __future__ import annotations

import argparse
import os
import sys

from .compare import compare_bundles, load_tolerances


def _default_tolerances_path() -> str:
    return os.path.join(os.path.dirname(__file__), "tolerances.yaml")


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="x13compare",
        description="Compare two X-13 golden bundles numerically.",
    )
    p.add_argument("bundle_a", help="path to first bundle directory")
    p.add_argument("bundle_b", help="path to second bundle directory")
    p.add_argument(
        "--tolerances",
        default=None,
        help="tolerance classification file (YAML/JSON). Defaults to the "
        "bundled tolerances.yaml if present.",
    )
    p.add_argument("--junit", default=None, help="write JUnit XML report here")
    p.add_argument("--summary", default=None, help="write text summary here")
    p.add_argument("--quiet", action="store_true", help="suppress stdout summary")
    return p


def main(argv=None) -> int:
    args = build_parser().parse_args(argv)

    tol_path = args.tolerances
    if tol_path is None:
        default = _default_tolerances_path()
        tol_path = default if os.path.exists(default) else None
    tolerances = load_tolerances(tol_path)

    report = compare_bundles(args.bundle_a, args.bundle_b, tolerances)

    if args.junit:
        report.write_junit(args.junit)
    if args.summary:
        report.write_text_summary(args.summary)
    if not args.quiet:
        print(report.to_text_summary())

    return report.exit_code


if __name__ == "__main__":  # pragma: no cover
    sys.exit(main())
