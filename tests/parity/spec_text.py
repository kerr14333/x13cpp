"""Read a `.spc` the way the PARSER sees it -- comments removed.

Several gates decide whether a spec is in scope by looking for tokens in its
text (`"automdl" in flat`, `"composite{" in flat`, ...). They read the raw
file, so a spec whose HEADER COMMENT happens to name one of those tokens
silently drops out of the gate -- no skip, no message, one fewer case in the
parametrisation. `extra/airline_estimate-maxiter-noconverge` is the case that
found it: a hand-authored spec with no `automdl{}` in it at all, excluded from
test_m3_estimate because its comment says which sibling spec covers the
automdl arm.

That is the "discovery predicate is a hand-written case list that has learned
to hide" failure with an extra twist -- here the spec's own documentation
disables its gates, so the more carefully a spec is commented the less of it
is tested. X-13 comments run from `#` to end of line (gtline.f); the parser
never sees them, and neither should a scope predicate.
"""
from __future__ import annotations

import os
import re

_COMMENT = re.compile(r"#.*")


def spec_body(path: str) -> str:
    """The spec's text with every `#` comment stripped."""
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        return "\n".join(_COMMENT.sub("", ln) for ln in fh.read().splitlines())


def spec_flat(path: str) -> str:
    """`spec_body` lower-cased with all spaces removed -- the form the token
    tests below want."""
    return spec_body(path).lower().replace(" ", "")


def _self_test_corpus_root() -> str:
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.abspath(os.path.join(here, "..", "corpus"))
