"""Call the X-13ARIMA-SEATS engine from Python, in-process, with no build step.

This is deliberately NOT a package. It is one file that opens the `x13c` shared
library (built by CMake from `core/src/api/x13_capi.cpp`) with `ctypes` and hands
back plain Python values. Copy it next to your script, or add this directory to
`sys.path`.

    import x13c
    run = x13c.adjust("tests/corpus/generated/airline_x11-default.spc")
    print(run.arima_model, run.period, run.nobs)
    sa = run.table("d11")            # -> Table(dates=[(1949,1),...], values=[...])
    print(sa.values[:5])
    print(run.diagnostics()["f3.q"])

Design notes worth knowing:

* Nothing is written to disk. That is the engine's standing rule and this layer
  keeps it: a run produces values, never files.
* Tables do NOT share one start date. Seasonal factors (`d10`) are projected a
  year past the data and backcasts precede it, so every `Table` carries its own
  dates. Do not assume `d10` and `d11` line up positionally -- join on dates.
* `adjust()` is NOT thread-safe when given a path, because resolving the spec's
  relative data paths means changing the process working directory for the
  duration of the run. Use `adjust_text()` with absolute paths inside the spec
  if you need concurrency.
* A failed run raises `X13Error`. Use `adjust(..., check=False)` to get the
  `Run` back anyway and inspect `run.ok` / `run.error`.
"""

from __future__ import annotations

import ctypes
import os
import sys
from typing import Dict, List, NamedTuple, Optional, Sequence, Tuple

__all__ = ["adjust", "adjust_text", "Run", "Table", "X13Error", "load_library"]


class X13Error(RuntimeError):
    """The engine reported a fatal condition, or the library could not be used."""


class Table(NamedTuple):
    """One named output series with its own calendar labels."""

    name: str
    dates: List[Tuple[int, int]]   # (year, period), period 1-based
    values: List[float]

    def __len__(self) -> int:
        return len(self.values)

    def as_dict(self) -> Dict[Tuple[int, int], float]:
        """{(year, period): value} -- convenient for joining tables by date."""
        return dict(zip(self.dates, self.values))


# --- library loading -------------------------------------------------------

_LIB = None


def _candidate_paths() -> List[str]:
    env = os.environ.get("X13C_LIBRARY")
    if env:
        return [env]
    here = os.path.dirname(os.path.abspath(__file__))
    repo = os.path.abspath(os.path.join(here, "..", ".."))
    names = (
        ["x13c.dll", "libx13c.dll"] if sys.platform == "win32"
        else ["libx13c.dylib"] if sys.platform == "darwin"
        else ["libx13c.so"]
    )
    dirs = [os.path.join(repo, "build"), here, os.getcwd()]
    return [os.path.join(d, n) for d in dirs for n in names]


def load_library(path: Optional[str] = None) -> ctypes.CDLL:
    """Open the shared library (cached). Set $X13C_LIBRARY to override the search."""
    global _LIB
    if _LIB is not None and path is None:
        return _LIB

    tried = [path] if path else _candidate_paths()
    last = None
    for p in tried:
        if p and os.path.exists(p):
            try:
                lib = ctypes.CDLL(p)
            except OSError as e:      # right name, wrong architecture / deps
                last = e
                continue
            _bind(lib)
            if path is None:
                _LIB = lib
            return lib
    raise X13Error(
        "could not load the x13c shared library. Build it with "
        "`tools/build.ps1` (target x13c), or set $X13C_LIBRARY to its path. "
        f"Tried: {tried}" + (f" (last error: {last})" if last else "")
    )


def _bind(lib: ctypes.CDLL) -> None:
    """Declare argtypes/restypes. Without this, ctypes assumes int returns and
    silently truncates every pointer on 64-bit -- the classic ctypes crash."""
    c, cp, ci, cd = ctypes.c_char_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_double
    sigs = {
        "x13_abi_version": ([], ci),
        "x13_engine_version": ([], c),
        "x13_run_spec_text": ([c, c], cp),
        "x13_run_spec_file": ([c], cp),
        "x13_run_free": ([cp], None),
        "x13_ok": ([cp], ci),
        "x13_error": ([cp], c),
        "x13_period": ([cp], ci),
        "x13_nobs": ([cp], ci),
        "x13_model_based": ([cp], ci),
        "x13_mode": ([cp], ci),
        "x13_arima_model": ([cp], c),
        "x13_table_count": ([cp], ci),
        "x13_table_name": ([cp, ci], c),
        "x13_table_length": ([cp, c], ci),
        "x13_table_start_year": ([cp, c], ci),
        "x13_table_start_period": ([cp, c], ci),
        "x13_table_values": ([cp, c, ctypes.POINTER(cd), ci], ci),
        "x13_table_dates": ([cp, c, ctypes.POINTER(ci), ctypes.POINTER(ci), ci], ci),
        "x13_diag_count": ([cp], ci),
        "x13_diag_name": ([cp, ci], c),
        "x13_diag_value": ([cp, c, ctypes.POINTER(cd)], ci),
    }
    for name, (args, res) in sigs.items():
        fn = getattr(lib, name)
        fn.argtypes = args
        fn.restype = res


MODES = {0: "multiplicative", 1: "additive", 2: "log-additive", 3: "pseudo-additive"}


class Run:
    """The outcome of one engine run. Owns a native handle; free it with
    `close()` or use it as a context manager. Reading a closed Run raises."""

    def __init__(self, lib: ctypes.CDLL, handle: int):
        self._lib = lib
        self._h = handle

    # -- lifetime ----------------------------------------------------------
    def close(self) -> None:
        if getattr(self, "_h", None):
            self._lib.x13_run_free(self._h)
            self._h = None

    def __enter__(self) -> "Run":
        return self

    def __exit__(self, *exc) -> None:
        self.close()

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass

    def _check(self):
        if not getattr(self, "_h", None):
            raise X13Error("this Run has been closed")
        return self._h

    # -- status ------------------------------------------------------------
    @property
    def ok(self) -> bool:
        return bool(self._lib.x13_ok(self._check()))

    @property
    def error(self) -> str:
        return self._lib.x13_error(self._check()).decode("utf-8", "replace")

    # -- metadata ----------------------------------------------------------
    @property
    def period(self) -> int:
        """Observations per year: 12 monthly, 4 quarterly."""
        return self._lib.x13_period(self._check())

    @property
    def nobs(self) -> int:
        return self._lib.x13_nobs(self._check())

    @property
    def model_based(self) -> bool:
        return bool(self._lib.x13_model_based(self._check()))

    @property
    def mode(self) -> str:
        return MODES.get(self._lib.x13_mode(self._check()), "unknown")

    @property
    def arima_model(self) -> str:
        return self._lib.x13_arima_model(self._check()).decode("utf-8", "replace")

    # -- tables ------------------------------------------------------------
    def table_names(self) -> List[str]:
        h = self._check()
        n = self._lib.x13_table_count(h)
        return [self._lib.x13_table_name(h, i).decode() for i in range(n)]

    def has_table(self, name: str) -> bool:
        return self._lib.x13_table_length(self._check(), name.encode()) > 0

    def table(self, name: str) -> Table:
        h = self._check()
        key = name.encode()
        n = self._lib.x13_table_length(h, key)
        if n <= 0:
            raise KeyError(
                f"this run produced no table {name!r} "
                f"(available: {', '.join(self.table_names()) or 'none'})"
            )
        vals = (ctypes.c_double * n)()
        got = self._lib.x13_table_values(h, key, vals, n)
        if got != n:
            raise X13Error(f"table {name!r}: expected {n} values, got {got}")
        yrs = (ctypes.c_int * n)()
        pers = (ctypes.c_int * n)()
        self._lib.x13_table_dates(h, key, yrs, pers, n)
        return Table(name, list(zip(list(yrs), list(pers))), list(vals))

    def tables(self, names: Optional[Sequence[str]] = None) -> Dict[str, Table]:
        """Every table (or just `names`) as a dict."""
        want = list(names) if names is not None else self.table_names()
        return {n: self.table(n) for n in want}

    # -- diagnostics -------------------------------------------------------
    def diagnostics(self) -> Dict[str, float]:
        """The scalar quality statistics (`f3.q`, `f3.m01`.., `f2.ic`, ...)."""
        h = self._check()
        out: Dict[str, float] = {}
        val = ctypes.c_double()
        for i in range(self._lib.x13_diag_count(h)):
            nm = self._lib.x13_diag_name(h, i).decode()
            if self._lib.x13_diag_value(h, nm.encode(), ctypes.byref(val)):
                out[nm] = val.value
        return out

    def __repr__(self) -> str:
        if not getattr(self, "_h", None):
            return "<x13c.Run closed>"
        if not self.ok:
            return f"<x13c.Run FAILED: {self.error}>"
        return (f"<x13c.Run {self.nobs} obs, period={self.period}, "
                f"mode={self.mode}, model={self.arima_model or 'none'}, "
                f"{len(self.table_names())} tables>")


def _finish(lib, handle, check: bool, what: str) -> Run:
    if not handle:
        raise X13Error(f"engine could not allocate a run for {what}")
    run = Run(lib, handle)
    if check and not run.ok:
        msg = run.error
        run.close()
        raise X13Error(msg)
    return run


def adjust(spec_path: str, *, check: bool = True,
           library: Optional[str] = None) -> Run:
    """Run the spec file at `spec_path`.

    Relative data paths inside the spec resolve against the spec's own
    directory. Raises `X13Error` on failure unless `check=False`.
    """
    lib = load_library(library)
    h = lib.x13_run_spec_file(os.fspath(spec_path).encode("utf-8"))
    return _finish(lib, h, check, repr(spec_path))


def adjust_text(spec_text: str, series_name: str = "series", *,
                check: bool = True, library: Optional[str] = None) -> Run:
    """Run a spec given as text. Relative data paths inside it resolve against
    the CURRENT working directory -- prefer absolute paths here."""
    lib = load_library(library)
    h = lib.x13_run_spec_text(spec_text.encode("utf-8"),
                              series_name.encode("utf-8"))
    return _finish(lib, h, check, "the supplied spec text")


def engine_version(library: Optional[str] = None) -> str:
    return load_library(library).x13_engine_version().decode()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"usage: {os.path.basename(__file__)} <spec.spc> [table]")
        raise SystemExit(2)
    with adjust(sys.argv[1]) as r:
        print(r)
        tag = sys.argv[2] if len(sys.argv) > 2 else "d11"
        t = r.table(tag)
        for (y, p), v in list(zip(t.dates, t.values))[:12]:
            print(f"  {y}-{p:02d}  {v:.6f}")
        if len(t) > 12:
            print(f"  ... {len(t) - 12} more")
