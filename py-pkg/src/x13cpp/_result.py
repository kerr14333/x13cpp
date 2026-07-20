"""The X13cpp result object.

:func:`x13cpp.seasonal_adjust` returns an :class:`X13Result`. It is a plain
dataclass with a fixed set of fields, described below, plus the accessor
methods on this class. Nothing about a result object writes to disk on its
own -- see :meth:`X13Result.write_outputs`.
"""

from __future__ import annotations

import csv
import os
from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional

from x13cpp._timeseries import TimeSeries


@dataclass
class X13Result:
    """Result of a :func:`x13cpp.seasonal_adjust` run.

    Attributes:
        call: A short human-readable description of the call that produced
            this result (function name + key arguments).
        series: The original input series.
        spec: The resolved spec dict used for the run (mirrors an X-13
            ``.spc`` file's structure).
        udg: A dict of scalar diagnostics, mirroring the ``.udg`` (user
            diagnostics) file X-13 writes -- e.g. ``f3.m01`` (M1 quality
            statistic), ``nobs``, ``aicc``, and so on. Empty until the core
            is wired in.
        fct: A forecast table as a :class:`TimeSeries`, or `None` if the
            run did not request forecasts (or the core is not wired in
            yet).
        save: A dict of "save tables", keyed by the X-13 table code
            (``"d10"``, ``"d11"``, ``"d12"``, ``"d13"``, ``"s10"``,
            ``"s11"``, ...), each a :class:`TimeSeries`. Empty until the
            core is wired in.
        status: Either ``"stub"`` (core not yet wired in) or ``"ok"``.
    """

    call: str
    series: Optional[TimeSeries] = None
    spec: Dict[str, Any] = field(default_factory=dict)
    udg: Dict[str, Any] = field(default_factory=dict)
    fct: Optional[TimeSeries] = None
    save: Dict[str, TimeSeries] = field(default_factory=dict)
    status: str = "stub"

    def __repr__(self) -> str:  # pragma: no cover - cosmetic
        stub_tag = " [STUB - core not wired in]" if self.status == "stub" else ""
        n_obs = len(self.series) if self.series is not None else 0
        return (
            f"<X13Result{stub_tag} call={self.call!r} n_obs={n_obs} "
            f"udg={len(self.udg)} fct={'yes' if self.fct is not None else 'no'} "
            f"save={list(self.save.keys())}>"
        )

    # -- accessors ---------------------------------------------------

    def get_udg(self) -> Dict[str, Any]:
        """Return the scalar diagnostics dict (mirrors X-13's .udg file)."""
        return self.udg

    def get_fct(self) -> Optional[TimeSeries]:
        """Return the forecast table, or `None` if there isn't one."""
        return self.fct

    def save_table(self, table: str) -> TimeSeries:
        """Return one save table by X-13 table code, e.g. ``"d11"``.

        Raises:
            KeyError: if `table` is not present in this result.
        """
        if table not in self.save:
            available = ", ".join(self.save) or "(none)"
            raise KeyError(
                f'Save table "{table}" is not present in this result. '
                f"Available tables: {available}"
            )
        return self.save[table]

    def seasadj(self) -> TimeSeries:
        """Convenience wrapper for the seasonally adjusted series (``"d11"``)."""
        return self.save_table("d11")

    def trend(self) -> TimeSeries:
        """Convenience wrapper for the trend component (``"d12"``)."""
        return self.save_table("d12")

    def irregular(self) -> TimeSeries:
        """Convenience wrapper for the irregular component (``"d13"``)."""
        return self.save_table("d13")

    # -- the one, explicit way to write files -------------------------

    def write_outputs(
        self,
        path: str,
        basename: str = "series",
        overwrite: bool = False,
    ) -> str:
        """Write this result's contents to disk.

        This is the *only* way an :class:`X13Result` ever touches the
        filesystem -- :func:`x13cpp.seasonal_adjust` itself never writes
        files. Mirrors the file layout the Census X-13ARIMA-SEATS program
        writes (one file per save table, plus a ``.udg`` diagnostics file
        and a ``.fct`` forecast file), so output can be diffed against the
        Fortran oracle's output if desired.

        Args:
            path: Directory to write into. Created if it does not exist.
            basename: File basename (without extension).
            overwrite: If `False` (default), raise if any target file
                already exists rather than silently overwriting it.

        Returns:
            `path`.
        """
        os.makedirs(path, exist_ok=True)

        candidates: List[str] = []
        if self.udg:
            candidates.append(".udg")
        if self.fct is not None:
            candidates.append(".fct")
        candidates.extend(f".{table}" for table in self.save)

        clashes = [
            ext for ext in candidates
            if os.path.exists(os.path.join(path, basename + ext))
        ]
        if clashes and not overwrite:
            targets = ", ".join(os.path.join(path, basename + ext) for ext in clashes)
            raise FileExistsError(
                f"Refusing to overwrite existing file(s): {targets}. "
                "Pass overwrite=True to replace them."
            )

        written = []

        if self.udg:
            udg_path = os.path.join(path, basename + ".udg")
            with open(udg_path, "w", encoding="ascii", newline="\n") as fh:
                for key, value in self.udg.items():
                    fh.write(f"{key}: {value}\n")
            written.append(udg_path)

        if self.fct is not None:
            fct_path = os.path.join(path, basename + ".fct")
            _write_timeseries_csv(self.fct, fct_path)
            written.append(fct_path)

        for table, ts in self.save.items():
            table_path = os.path.join(path, f"{basename}.{table}")
            _write_timeseries_csv(ts, table_path)
            written.append(table_path)

        return path


def _write_timeseries_csv(ts: TimeSeries, path: str) -> None:
    """Write a TimeSeries to a two-column (period, value) CSV file."""
    year, period = ts.start
    with open(path, "w", encoding="ascii", newline="") as fh:
        writer = csv.writer(fh)
        writer.writerow(["period", "value"])
        y, p = year, period
        for v in ts.values:
            writer.writerow([f"{y}.{p:02d}", v])
            p += 1
            if p > ts.freq:
                p = 1
                y += 1
