#!/usr/bin/env python
"""Run the X-13ARIMA-SEATS oracle binary and collect a golden output bundle.

Usage
-----
    python run_oracle.py --binary PATH --spec SPEC.spc [--outdir DIR]
                         [--flags "-s -w"] [--timeout SECONDS]

Behaviour
---------
1. Create a clean bundle directory (``--outdir``, default ``<specbase>.bundle``
   next to the spec).
2. Copy the spec file and any data files it references into the bundle dir
   (that dir doubles as the run's working directory, so relative ``file=``
   references resolve).
3. Invoke the binary. The binary is passed the spec *base name* (no ``.spc``)
   as the trailing positional argument, plus any extra ``--flags``. This matches
   the X-13 command line ("``-i infile`` or ``infile``"); ``-s`` additionally
   writes the machine-readable ``.udg`` diagnostics DB.
4. Collect every produced output (``.out`` / ``.err`` / ``.log`` / ``.udg`` and
   all save tables) and write ``manifest.json`` recording the file list with
   sha256, the binary path + sha256 + detected version, the flags, the exact
   command, a UTC timestamp and the process exit code.

Robust to a nonzero exit: the exit code is recorded and outputs are still
collected. The module is import-friendly -- :func:`run_oracle` returns a
:class:`RunResult` for use by the parity harness.

NOTE (verify against real binary): the invocation form (positional spec base
name, outputs sharing that base name) and the set of files the binary drops in
the run directory are assumptions documented here; adjust once the compiled
binary is available.
"""

from __future__ import annotations

import argparse
import datetime as _dt
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from typing import Dict, List, Optional


# Extensions treated as run *inputs* (copied in) rather than collected outputs.
INPUT_EXTS = {".spc", ".dta", ".dat", ".txt", ".prn"}

# Regex for data-file references inside a spec (file="..." / file = name).
_FILE_REF_RE = re.compile(r"""\bfile\s*=\s*["']?([^"'\s{}]+)["']?""", re.IGNORECASE)


class OracleRunError(RuntimeError):
    pass


@dataclass
class RunResult:
    spec: str
    bundle_dir: str
    command: List[str]
    exit_code: int
    binary: str
    binary_sha256: str
    binary_version: str
    flags: List[str]
    timestamp: str
    outputs: List[str] = field(default_factory=list)      # output basenames
    inputs: List[str] = field(default_factory=list)       # copied-in basenames
    manifest_path: str = ""
    stdout: str = ""
    stderr: str = ""

    @property
    def out_file(self) -> Optional[str]:
        for name in self.outputs:
            if name.lower().endswith(".out"):
                return os.path.join(self.bundle_dir, name)
        return None


def sha256_of(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def _referenced_data_files(spec_path: str) -> List[str]:
    """Return absolute paths of data files referenced by a spec that exist."""
    spec_dir = os.path.dirname(os.path.abspath(spec_path))
    try:
        with open(spec_path, "r", encoding="utf-8", errors="replace") as fh:
            text = fh.read()
    except OSError:
        return []
    found: List[str] = []
    seen = set()
    for m in _FILE_REF_RE.finditer(text):
        ref = m.group(1).strip()
        if not ref:
            continue
        cand = ref if os.path.isabs(ref) else os.path.join(spec_dir, ref)
        cand = os.path.normpath(cand)
        if cand in seen:
            continue
        seen.add(cand)
        if os.path.isfile(cand):
            found.append(cand)
    return found


def _detect_version(bundle_dir: str, out_basename: str, stdout: str) -> str:
    """Best-effort extraction of the binary version string."""
    candidates = []
    out_path = os.path.join(bundle_dir, out_basename)
    if os.path.isfile(out_path):
        try:
            with open(out_path, "r", encoding="utf-8", errors="replace") as fh:
                head = fh.read(8000)
            candidates.append(head)
        except OSError:
            pass
    candidates.append(stdout or "")
    ver_re = re.compile(r"(?:Version|X-?13[A-Z-]*)\s*[:v]?\s*([0-9][0-9A-Za-z.\- ]+)",
                        re.IGNORECASE)
    for text in candidates:
        for line in text.splitlines():
            if "version" in line.lower() or "build" in line.lower():
                m = ver_re.search(line)
                if m:
                    return line.strip()[:120]
    return "unknown"


def run_oracle(
    binary: str,
    spec: str,
    outdir: Optional[str] = None,
    flags: Optional[List[str]] = None,
    timeout: Optional[float] = None,
    extra_inputs: Optional[List[str]] = None,
    clean: bool = True,
) -> RunResult:
    """Run the oracle binary on *spec* and collect a golden bundle.

    Parameters
    ----------
    binary : path to the oracle executable (may not need to exist until call).
    spec   : path to the ``.spc`` file.
    outdir : bundle directory (created clean). Defaults to ``<specbase>.bundle``.
    flags  : extra command-line flags (list of tokens), e.g. ``["-s", "-w"]``.
    timeout: seconds before the subprocess is killed (None = no limit).
    extra_inputs : additional files to copy into the run dir (e.g. metafiles).
    clean  : if True, wipe an existing outdir first.
    """
    binary = os.path.abspath(binary)
    spec = os.path.abspath(spec)
    if not os.path.isfile(spec):
        raise OracleRunError(f"spec file not found: {spec}")

    spec_base = os.path.splitext(os.path.basename(spec))[0]
    if outdir is None:
        outdir = os.path.join(os.path.dirname(spec), spec_base + ".bundle")
    outdir = os.path.abspath(outdir)

    if clean and os.path.isdir(outdir):
        shutil.rmtree(outdir)
    os.makedirs(outdir, exist_ok=True)

    # Copy inputs into the (clean) run/bundle dir. Data files land at the
    # bundle root, so ``file=`` references in the copied spec are rewritten to
    # their basenames — the binary runs with cwd=bundle and would otherwise
    # fail on paths like ``../data/airline.dat`` that were relative to the
    # spec's original location.
    copied_inputs: List[str] = []
    dst_spec = os.path.join(outdir, os.path.basename(spec))
    spec_text = open(spec, encoding="utf-8", errors="replace").read()
    for data_path in _referenced_data_files(spec):
        dst = os.path.join(outdir, os.path.basename(data_path))
        if os.path.abspath(dst) != os.path.abspath(data_path):
            shutil.copy2(data_path, dst)
        copied_inputs.append(os.path.basename(data_path))

    def _rewrite_ref(m: "re.Match[str]") -> str:
        ref = m.group(1)
        base = os.path.basename(ref.replace("\\", "/"))
        return m.group(0).replace(ref, base)

    spec_text = _FILE_REF_RE.sub(_rewrite_ref, spec_text)
    with open(dst_spec, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(spec_text)
    shutil.copystat(spec, dst_spec)
    copied_inputs.append(os.path.basename(spec))
    for extra in extra_inputs or []:
        if os.path.isfile(extra):
            shutil.copy2(extra, os.path.join(outdir, os.path.basename(extra)))
            copied_inputs.append(os.path.basename(extra))

    flags = list(flags or [])
    # Command: binary <specbase> <flags>   (run with cwd = outdir).
    # The spec base must come FIRST: x13as treats a bare name as the input
    # spec only in the first position; after a flag it errors with
    # "Program option <name> not defined".
    command = [binary, spec_base] + flags

    timestamp = _dt.datetime.now(_dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    # Snapshot files present before the run so we can identify new outputs.
    before = set(os.listdir(outdir))

    exit_code = -1
    stdout = stderr = ""
    binary_missing = not os.path.isfile(binary)
    if binary_missing:
        # Don't raise: record the condition so the harness can proceed/skip.
        stderr = f"binary not found: {binary}"
        exit_code = 127
    else:
        try:
            proc = subprocess.run(
                command,
                cwd=outdir,
                capture_output=True,
                text=True,
                timeout=timeout,
            )
            exit_code = proc.returncode
            stdout = proc.stdout or ""
            stderr = proc.stderr or ""
        except subprocess.TimeoutExpired as exc:
            exit_code = -9
            stdout = (exc.stdout or b"").decode("utf-8", "replace") if isinstance(
                exc.stdout, bytes) else (exc.stdout or "")
            stderr = (
                (exc.stderr or b"").decode("utf-8", "replace")
                if isinstance(exc.stderr, bytes) else (exc.stderr or "")
            ) + f"\n[timeout after {timeout}s]"
        except OSError as exc:
            exit_code = 126
            stderr = f"failed to launch binary: {exc}"

    # Persist captured stdout/stderr for provenance.
    if stdout:
        with open(os.path.join(outdir, spec_base + ".stdout.txt"), "w",
                  encoding="utf-8") as fh:
            fh.write(stdout)
    if stderr:
        with open(os.path.join(outdir, spec_base + ".stderr.txt"), "w",
                  encoding="utf-8") as fh:
            fh.write(stderr)

    # Classify files now in the bundle dir.
    after = set(os.listdir(outdir))
    input_names = set(copied_inputs)
    outputs: List[str] = []
    inputs: List[str] = []
    manifest_files: List[Dict[str, object]] = []
    for name in sorted(after):
        full = os.path.join(outdir, name)
        if not os.path.isfile(full):
            continue
        if name == "manifest.json":
            continue
        entry = {
            "name": name,
            "sha256": sha256_of(full),
            "size": os.path.getsize(full),
            "role": "input" if name in input_names else "output",
        }
        manifest_files.append(entry)
        if name in input_names:
            inputs.append(name)
        else:
            outputs.append(name)

    binary_sha = sha256_of(binary) if not binary_missing else ""
    version = _detect_version(outdir, spec_base + ".out", stdout)

    manifest = {
        "spec": os.path.basename(spec),
        "spec_base": spec_base,
        "command": command,
        "cwd": outdir,
        "flags": flags,
        "exit_code": exit_code,
        "timestamp_utc": timestamp,
        "binary": binary,
        "binary_sha256": binary_sha,
        "binary_version": version,
        "binary_missing": binary_missing,
        "files": manifest_files,
    }
    manifest_path = os.path.join(outdir, "manifest.json")
    with open(manifest_path, "w", encoding="utf-8") as fh:
        json.dump(manifest, fh, indent=2, sort_keys=True)

    return RunResult(
        spec=spec,
        bundle_dir=outdir,
        command=command,
        exit_code=exit_code,
        binary=binary,
        binary_sha256=binary_sha,
        binary_version=version,
        flags=flags,
        timestamp=timestamp,
        outputs=outputs,
        inputs=inputs,
        manifest_path=manifest_path,
        stdout=stdout,
        stderr=stderr,
    )


def _parse_flags(flag_str: Optional[str]) -> List[str]:
    if not flag_str:
        return []
    import shlex
    return shlex.split(flag_str)


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="run_oracle.py",
        description="Run the X-13 oracle binary and collect a golden bundle.",
    )
    p.add_argument("--binary", required=True, help="path to the oracle executable")
    p.add_argument("--spec", required=True, help="path to the .spc spec file")
    p.add_argument("--outdir", default=None, help="bundle output directory")
    p.add_argument("--flags", default=None,
                   help='extra binary flags as one string, e.g. "-s -w"')
    p.add_argument("--timeout", type=float, default=None,
                   help="seconds before the run is killed")
    p.add_argument("--no-clean", action="store_true",
                   help="do not wipe an existing outdir first")
    return p


def main(argv=None) -> int:
    args = build_parser().parse_args(argv)
    result = run_oracle(
        binary=args.binary,
        spec=args.spec,
        outdir=args.outdir,
        flags=_parse_flags(args.flags),
        timeout=args.timeout,
        clean=not args.no_clean,
    )
    print(f"bundle:   {result.bundle_dir}")
    print(f"command:  {' '.join(result.command)}")
    print(f"exit:     {result.exit_code}")
    print(f"version:  {result.binary_version}")
    print(f"outputs:  {len(result.outputs)} files, inputs: {len(result.inputs)}")
    print(f"manifest: {result.manifest_path}")
    # Nonzero oracle exit is reported but not treated as a harness failure here;
    # the bundle (with recorded exit code) is the artefact of interest.
    return 0


if __name__ == "__main__":
    sys.exit(main())
