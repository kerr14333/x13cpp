"""Did the ORACLE's run halt, and if so, was it a rejection or a late refusal?

Shared because five phase gates used to assume the same wrong thing:
`assert harness.returncode == 0`. That reads as "the run worked", and it held
for every spec in the corpus only because every erroring spec failed EARLY --
so "the oracle errored" and "there is nothing to compare" were the same
condition by coincidence, never by rule.

`extra/airline_slidingspans-x11regression-user-nofixx11reg` separates them. It
parses, fits, runs the whole main X-11 pass, writes a full `.udg` and every D
table, and only then halts inside sliding span #2 on a singular
irregular-regression design (`prterx`). The oracle punched its tables before the
span driver ran; this engine dumps them at exit and returns 1. A gate that
demands exit 0 cannot express that run at all -- which is entry 85's lesson
arriving from the other side: **a late refusal has a complete run behind it, and
throwing the run away because it ends in a refusal is how you fail to gate it.**

`expected_exit` is therefore an EQUALITY, not a relaxation: where the oracle
halted, the engine is required to halt too, and its tables are still compared.
"""
import json
import os


def oracle_halted(golden_dir: str, base: str) -> bool:
    """True when the oracle's run ended in an ERROR, whatever phase raised it."""
    errf = os.path.join(golden_dir, base + ".err")
    if not os.path.exists(errf):
        return False
    with open(errf, encoding="utf-8", errors="replace") as fh:
        return any(l.lstrip().startswith("ERROR:") for l in fh)


def oracle_reported(golden_dir: str, base: str) -> bool:
    """True when the oracle got far enough to write a real `.udg`.

    NON-EMPTY is the operative word: the oracle opens the file before it
    validates, so a spec rejected at parse ships a zero-byte `.udg` while one
    that ran and halted later ships a full one.
    """
    udg = os.path.join(golden_dir, base + ".udg")
    return os.path.exists(udg) and os.path.getsize(udg) > 0


def expected_exit(golden_dir: str, base: str) -> int:
    """The exit code this engine's phase harnesses must return for that spec."""
    return 1 if oracle_halted(golden_dir, base) else 0


def exit_message(base: str, expected: int, actual: int, stderr: str) -> str:
    what = ("the oracle HALTED on this spec, so the engine must too"
            if expected else "the oracle completed this spec")
    return f"{base}: {what} -- expected exit {expected}, got {actual}\n{stderr}"
