import os

import pytest

from x13compare import compare
from x13compare.compare import Tolerances, compare_bundles, load_tolerances

PKG_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "x13compare"))
TOL_YAML = os.path.join(PKG_DIR, "tolerances.yaml")


def _write(path, text):
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(text)


def _save_text(label, pairs):
    lines = [f"date\t{label}", "------\t-----------"]
    for d, v in pairs:
        lines.append(f"{d}\t{v:.12g}")
    return "\n".join(lines) + "\n"


def _make_bundle(root, name, files):
    d = os.path.join(root, name)
    os.makedirs(d, exist_ok=True)
    for fn, text in files.items():
        _write(os.path.join(d, fn), text)
    return d


# --------------------------------------------------------------------------- #
def test_identical_bundles_pass(tmp_path):
    files = {"s.d11": _save_text("S.d11", [(199001, 100.0), (199002, 200.0)])}
    a = _make_bundle(tmp_path, "a", files)
    b = _make_bundle(tmp_path, "b", files)
    report = compare_bundles(a, b, Tolerances())
    assert report.passed
    assert report.exit_code == 0
    assert report.total_compared == 2


def test_perturbation_1e7_caught_at_default_1e8(tmp_path):
    base = [(199001, 100.0), (199002, 200.0)]
    pert = [(199001, 100.0 * (1 + 1e-7)), (199002, 200.0)]
    a = _make_bundle(tmp_path, "a", {"s.d11": _save_text("S.d11", base)})
    b = _make_bundle(tmp_path, "b", {"s.d11": _save_text("S.d11", pert)})
    report = compare_bundles(a, b, Tolerances())  # default rtol 1e-8
    assert not report.passed
    assert report.exit_code == 1
    nums = [m for m in report.mismatches if m.kind == "numeric"]
    assert len(nums) == 1
    assert nums[0].key == "199001"


def test_perturbation_1e7_passes_for_optimizer_class(tmp_path):
    # Same 1e-7 relative perturbation, but on an optimizer-class table (fct)
    # compared at rtol 1e-6 -> must pass.
    base = [(199001, 100.0), (199002, 200.0)]
    pert = [(199001, 100.0 * (1 + 1e-7)), (199002, 200.0)]
    a = _make_bundle(tmp_path, "a", {"s.fct": _save_text("S.fct", base)})
    b = _make_bundle(tmp_path, "b", {"s.fct": _save_text("S.fct", pert)})
    tol = load_tolerances(TOL_YAML)
    assert tol.resolve("fct")[2] == "optimizer_dependent"
    report = compare_bundles(a, b, tol)
    assert report.passed, report.to_text_summary()


def test_optimizer_class_still_catches_larger_error(tmp_path):
    # A 1e-4 relative error must still fail even at 1e-6 tolerance.
    base = [(199001, 100.0)]
    pert = [(199001, 100.0 * (1 + 1e-4))]
    a = _make_bundle(tmp_path, "a", {"s.fct": _save_text("S.fct", base)})
    b = _make_bundle(tmp_path, "b", {"s.fct": _save_text("S.fct", pert)})
    report = compare_bundles(a, b, load_tolerances(TOL_YAML))
    assert not report.passed


def test_out_table_numeric_mismatch(tmp_path):
    out_a = (
        " D 11  Final seasonally adjusted data\n"
        "  Observations        1\n"
        "----------\n"
        " Year   Jan     Feb\n"
        "----------\n"
        " 1990  10.0    20.0\n"
    )
    out_b = out_a.replace("20.0", "20.0000005")  # 2.5e-8 relative on Feb
    a = _make_bundle(tmp_path, "a", {"r.out": out_a})
    b = _make_bundle(tmp_path, "b", {"r.out": out_b})
    report = compare_bundles(a, b, Tolerances())
    assert not report.passed
    mm = [m for m in report.mismatches if m.kind == "numeric"]
    assert mm and mm[0].section == "d11" and mm[0].key == "1990/Feb"


def test_out_text_line_mismatch(tmp_path):
    out_a = " D 11  Title\n  Observations 1\n 1990 1.0\nNote alpha here\n"
    out_b = " D 11  Title\n  Observations 1\n 1990 1.0\nNote beta here\n"
    a = _make_bundle(tmp_path, "a", {"r.out": out_a})
    b = _make_bundle(tmp_path, "b", {"r.out": out_b})
    report = compare_bundles(a, b, Tolerances())
    assert not report.passed
    assert any(m.kind == "text" for m in report.mismatches)


def test_whitespace_normalised_text_passes(tmp_path):
    # Differing internal whitespace only -> text comparison must pass.
    out_a = " D 11  T\n  Observations 1\n 1990 1.0\nA   note   with spaces\n"
    out_b = " D 11  T\n  Observations 1\n 1990 1.0\nA note with spaces\n"
    a = _make_bundle(tmp_path, "a", {"r.out": out_a})
    b = _make_bundle(tmp_path, "b", {"r.out": out_b})
    report = compare_bundles(a, b, Tolerances())
    assert report.passed, report.to_text_summary()


def test_file_only_in_one_bundle(tmp_path):
    a = _make_bundle(tmp_path, "a", {"s.d11": _save_text("S.d11", [(199001, 1.0)])})
    b = _make_bundle(tmp_path, "b", {})
    report = compare_bundles(a, b, Tolerances())
    assert not report.passed
    statuses = {f.name: f.status for f in report.files}
    assert statuses["s.d11"] == "only_a"


def test_udg_numeric_and_string(tmp_path):
    a = _make_bundle(tmp_path, "a", {"r.udg": "aic: 100.0\nmodel: airline\n"})
    b = _make_bundle(tmp_path, "b", {"r.udg": "aic: 100.0000005\nmodel: airline\n"})
    report = compare_bundles(a, b, Tolerances())  # 5e-9 rel -> under 1e-8? no
    # |d| = 5e-8, atol+rtol*|b| = 1e-12 + 1e-8*100 = 1e-6 -> passes
    assert report.passed
    # Now change the string value -> must fail as text.
    c = _make_bundle(tmp_path, "c", {"r.udg": "aic: 100.0\nmodel: arima\n"})
    report2 = compare_bundles(a, c, Tolerances())
    assert not report2.passed


def test_junit_and_summary_writers(tmp_path):
    base = [(199001, 100.0)]
    pert = [(199001, 100.0 * (1 + 1e-3))]
    a = _make_bundle(tmp_path, "a", {"s.d11": _save_text("S.d11", base)})
    b = _make_bundle(tmp_path, "b", {"s.d11": _save_text("S.d11", pert)})
    report = compare_bundles(a, b, Tolerances())
    junit = report.to_junit_xml()
    assert junit.startswith("<?xml")
    assert "<testsuite" in junit and "<failure" in junit
    summary = report.to_text_summary()
    assert "FAIL" in summary
    jpath = os.path.join(tmp_path, "j.xml")
    spath = os.path.join(tmp_path, "s.txt")
    report.write_junit(jpath)
    report.write_text_summary(spath)
    assert os.path.getsize(jpath) > 0
    assert os.path.getsize(spath) > 0


def test_numbers_close_edge_cases():
    assert compare.numbers_close(1.0, 1.0, 1e-8, 1e-12)
    assert compare.numbers_close(0.0, 0.0, 1e-8, 1e-12)
    assert not compare.numbers_close(1.0, 1.0001, 1e-8, 1e-12)
    assert compare.numbers_close(float("nan"), float("nan"), 1e-8, 1e-12)
    assert not compare.numbers_close(float("inf"), 1.0, 1e-8, 1e-12)
