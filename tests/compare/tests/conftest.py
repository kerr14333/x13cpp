"""Make the ``x13compare`` package importable when running pytest from anywhere.

Adds ``tests/compare`` (the directory that *contains* the package) to sys.path.
"""

import os
import sys

_COMPARE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if _COMPARE_DIR not in sys.path:
    sys.path.insert(0, _COMPARE_DIR)

FIXTURES = os.path.join(os.path.dirname(__file__), "fixtures")


def fixture(name):
    return os.path.join(FIXTURES, name)
