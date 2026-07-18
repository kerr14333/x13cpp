"""Shared fixed-form Fortran source preprocessing for the X13cpp generators.

Handles the lexical layer common to prm2hpp / cmn2hpp / f2skel:
  * comment lines (c / C / * / ! in column 1, or blank),
  * fixed-form continuation (any non-blank, non-'0' char in column 6),
  * concatenation of continuation lines with NO implied blank (so identifiers or
    quoted strings split across a continuation are rejoined exactly, per the
    Fortran standard),
  * splitting a statement list on top-level commas while respecting parentheses
    and quoted strings.

Everything here is deliberately dependency-free (stdlib only).
"""
from __future__ import annotations

import re
from typing import List, Tuple


def is_comment(line: str) -> bool:
    if line.strip() == "":
        return True
    return line[0] in "cC*!"


def logical_statements(text: str) -> List[str]:
    """Collapse fixed-form source into a list of logical statement strings.

    Each returned string is columns 7.. of the (rejoined) statement, with the
    label field dropped and continuations concatenated directly.
    """
    out: List[str] = []
    cur: str | None = None
    for raw in text.splitlines():
        line = raw.rstrip("\n").rstrip()
        if line == "":
            continue
        if is_comment(line):
            continue
        if line[0] == "\t":
            # Tab-format source (VAX/gfortran extension): a leading tab replaces
            # the column 1-6 field. If the char after the tab is a digit 1-9 the
            # line is a continuation; otherwise it starts a new statement.
            after = line[1:]
            if after and after[0].isdigit() and after[0] != "0":
                cur = (cur or "") + after[1:]
            else:
                if cur is not None:
                    out.append(cur)
                cur = after
            continue
        # Column 6 (index 5) is the continuation marker in fixed form.
        cont = line[5] if len(line) > 5 else " "
        body = line[6:] if len(line) > 6 else ""
        if cont not in (" ", "0"):
            # Continuation: concatenate exactly (no inserted blank).
            cur = (cur or "") + body
        else:
            if cur is not None:
                out.append(cur)
            cur = body
    if cur is not None:
        out.append(cur)
    return out


def strip_trailing_comment(s: str) -> str:
    """Remove a trailing '!' comment that is not inside a quoted string."""
    in_s = False
    q = ""
    for i, ch in enumerate(s):
        if in_s:
            if ch == q:
                in_s = False
        else:
            if ch in "'\"":
                in_s = True
                q = ch
            elif ch == "!":
                return s[:i]
    return s


def split_top_level(s: str, sep: str = ",") -> List[str]:
    """Split on `sep` at paren depth 0, respecting quoted strings."""
    parts: List[str] = []
    depth = 0
    in_s = False
    q = ""
    cur = ""
    for ch in s:
        if in_s:
            cur += ch
            if ch == q:
                in_s = False
            continue
        if ch in "'\"":
            in_s = True
            q = ch
            cur += ch
        elif ch == "(":
            depth += 1
            cur += ch
        elif ch == ")":
            depth -= 1
            cur += ch
        elif ch == sep and depth == 0:
            parts.append(cur)
            cur = ""
        else:
            cur += ch
    if cur.strip() != "" or parts:
        parts.append(cur)
    return parts


def match_paren(s: str, open_idx: int) -> int:
    """Return index of the ')' matching the '(' at open_idx (respecting quotes)."""
    depth = 0
    in_s = False
    q = ""
    for i in range(open_idx, len(s)):
        ch = s[i]
        if in_s:
            if ch == q:
                in_s = False
            continue
        if ch in "'\"":
            in_s = True
            q = ch
        elif ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                return i
    return -1


def find_includes(text: str) -> List[str]:
    """Return the list of INCLUDEd file names (as written) in a Fortran source."""
    incs: List[str] = []
    for stmt in logical_statements(text):
        m = re.match(r"\s*include\s+['\"]([^'\"]+)['\"]", stmt, re.IGNORECASE)
        if m:
            incs.append(m.group(1))
    return incs


def has_equivalence(text: str) -> bool:
    for stmt in logical_statements(text):
        if re.match(r"\s*equivalence\b", stmt, re.IGNORECASE):
            return True
    return False
