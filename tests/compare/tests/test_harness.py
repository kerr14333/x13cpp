"""Tests for oracle/run_oracle.py and tests/parity/run_parity.py.

These use a tiny fake "binary" shim (a .bat on Windows / sh script on POSIX)
that fabricates output files, so they run without the real oracle.
"""

import importlib.util
import json
import os
import stat
import sys

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", "..", ".."))


def _load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    # Register before exec so @dataclass can resolve the module namespace.
    sys.modules[name] = mod
    spec.loader.exec_module(mod)
    return mod


run_oracle = _load(os.path.join(_REPO, "oracle", "run_oracle.py"), "run_oracle")
run_parity = _load(os.path.join(_REPO, "tests", "parity", "run_parity.py"),
                   "run_parity")


def _make_fake_binary(tmp_path):
    """A shim that writes <specbase>.out and <specbase>.udg in its cwd."""
    if os.name == "nt":
        p = tmp_path / "fakex13.bat"
        p.write_text(
            "@echo off\r\n"
            "echo  D 11  fake seasonally adjusted> %1.out\r\n"
            "echo  1990 1.0 2.0>> %1.out\r\n"
            "echo aic: 1.0> %1.udg\r\n",
            encoding="ascii",
        )
        return str(p)
    p = tmp_path / "fakex13.sh"
    p.write_text(
        "#!/bin/sh\n"
        'printf " D 11  fake seasonally adjusted\\n 1990 1.0 2.0\\n" > "$1.out"\n'
        'printf "aic: 1.0\\n" > "$1.udg"\n',
        encoding="ascii",
    )
    os.chmod(p, os.stat(p).st_mode | stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH)
    return str(p)


def _write_spec(tmp_path, name="sales.spc", with_data=True):
    d = tmp_path / "specdir"
    d.mkdir(exist_ok=True)
    if with_data:
        (d / "sales.dat").write_text("1 2 3 4\n", encoding="ascii")
        body = 'series{ title="s" file="sales.dat" start=1990.1 }\n'
    else:
        body = 'series{ title="s" data=(1 2 3 4) start=1990.1 }\n'
    spec = d / name
    spec.write_text(body, encoding="ascii")
    return str(spec)


# --------------------------------------------------------------------------- #
def test_run_oracle_missing_binary_records_exit(tmp_path):
    spec = _write_spec(tmp_path)
    out = tmp_path / "bundle"
    result = run_oracle.run_oracle(
        binary=str(tmp_path / "does_not_exist.exe"),
        spec=spec,
        outdir=str(out),
        flags=["-s"],
    )
    assert result.exit_code == 127
    assert os.path.isfile(result.manifest_path)
    manifest = json.loads(open(result.manifest_path, encoding="utf-8").read())
    assert manifest["binary_missing"] is True
    assert manifest["flags"] == ["-s"]
    # spec + referenced data file copied in as inputs
    assert "sales.spc" in result.inputs
    assert "sales.dat" in result.inputs


def test_run_oracle_data_reference_copied(tmp_path):
    spec = _write_spec(tmp_path, with_data=True)
    refs = run_oracle._referenced_data_files(spec)
    assert any(os.path.basename(r) == "sales.dat" for r in refs)


def test_run_oracle_real_run_collects_outputs(tmp_path):
    binary = _make_fake_binary(tmp_path)
    spec = _write_spec(tmp_path)
    out = tmp_path / "bundle"
    result = run_oracle.run_oracle(binary=binary, spec=spec,
                                   outdir=str(out), flags=[])
    assert result.exit_code == 0
    # fabricated outputs collected
    assert any(o.endswith(".out") for o in result.outputs)
    assert any(o.endswith(".udg") for o in result.outputs)
    # manifest lists sha256 for every file
    manifest = json.loads(open(result.manifest_path, encoding="utf-8").read())
    for entry in manifest["files"]:
        assert len(entry["sha256"]) == 64
        assert entry["role"] in ("input", "output")


# --------------------------------------------------------------------------- #
def test_discover_specs(tmp_path):
    corpus = tmp_path / "corpus"
    (corpus / "x11").mkdir(parents=True)
    (corpus / "x11" / "sales.spc").write_text("series{}\n", encoding="ascii")
    (corpus / "seats.spc").write_text("series{}\n", encoding="ascii")
    cases = run_parity.discover_specs(str(corpus))
    ids = sorted(c.rel_id for c in cases)
    assert ids == ["seats", "x11/sales"]


def test_cpp_engine_stub_raises():
    with pytest.raises(NotImplementedError):
        run_parity.CppEngine().run("x.spc", "out", [])


def test_parity_cpp_engine_marks_skipped(tmp_path):
    corpus = tmp_path / "corpus"
    corpus.mkdir()
    (corpus / "a.spc").write_text("series{}\n", encoding="ascii")
    summary = run_parity.run_parity(
        engine=run_parity.CppEngine(),
        corpus_dir=str(corpus),
        golden_dir=str(tmp_path / "golden"),
        runs_dir=str(tmp_path / "runs"),
        tolerances_path=None,
    )
    assert summary.outcomes[0].status == "skipped"
    assert summary.exit_code == 0  # skips are not failures


def test_parity_bless_then_compare_passes(tmp_path):
    binary = _make_fake_binary(tmp_path)
    corpus = tmp_path / "corpus"
    corpus.mkdir()
    # place a spec (with inline data so no external file needed)
    (corpus / "sales.spc").write_text(
        'series{ title="s" data=(1 2 3 4) start=1990.1 }\n', encoding="ascii")
    golden = tmp_path / "golden"
    runs = tmp_path / "runs"

    engine = run_parity.OracleEngine(binary)
    # bless
    blessed = run_parity.run_parity(engine, str(corpus), str(golden),
                                    str(runs), flags=[], tolerances_path=None,
                                    update=True)
    assert blessed.outcomes[0].status == "blessed"
    assert os.path.isdir(golden / "sales")

    # compare against the just-blessed golden -> deterministic shim -> pass
    checked = run_parity.run_parity(engine, str(corpus), str(golden),
                                    str(runs), flags=[], tolerances_path=None)
    assert checked.outcomes[0].status == "pass", checked.text()
    assert checked.exit_code == 0
