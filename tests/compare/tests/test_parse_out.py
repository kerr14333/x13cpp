import os

from x13compare import parse_out

FIXTURES = os.path.join(os.path.dirname(__file__), "fixtures")


def _load():
    return parse_out.parse_out_file(os.path.join(FIXTURES, "sample.out"))


def test_tables_discovered():
    po = _load()
    assert set(po.tables) == {"d11", "d10"}
    assert po.tables["d11"].title == "Final seasonally adjusted data"
    assert po.tables["d10"].title == "Final seasonal factors"


def test_monthly_columns_and_cells():
    po = _load()
    d11 = po.tables["d11"]
    assert d11.columns[:3] == ["Jan", "Feb", "Mar"]
    assert d11.columns[-1] == "Total"
    assert d11.cells[("1990", "Jan")] == 100.0
    assert d11.cells[("1990", "Total")] == 1266.0
    assert d11.cells[("1991", "Dec")] == 211.0


def test_pagination_tolerated():
    # 1991 row appears immediately after a form-feed page header; it must still
    # be parsed as data belonging to the same table.
    po = _load()
    assert po.tables["d11"].cells[("1991", "Jan")] == 200.0
    assert po.tables["d11"].cells[("1991", "Total")] == 2466.0


def test_summary_row_parsed():
    po = _load()
    d11 = po.tables["d11"]
    assert d11.cells[("AVGE", "Jan")] == 150.0
    assert d11.cells[("AVGE", "Dec")] == 161.0
    # No total column value for the AVGE row (only 12 values given).
    assert ("AVGE", "Total") not in d11.cells


def test_quarterly_table():
    po = _load()
    d10 = po.tables["d10"]
    assert d10.columns[:4] == ["1st", "2nd", "3rd", "4th"]
    assert d10.cells[("1990", "1st")] == 98.5
    assert d10.cells[("1991", "4th")] == 103.0


def test_text_lines_kept_separately():
    po = _load()
    assert any("plain text" in line for line in po.text_lines)
    # Page-header lines (PAGE / SERIES) are dropped from text comparison.
    assert not any("PAGE" in line for line in po.text_lines)


def test_no_header_falls_back_to_index_columns():
    text = (
        " B 1  Original series\n"
        "  Observations        2\n"
        "----------\n"
        " 1990  10.0  20.0  30.0\n"
    )
    po = parse_out.parse_out(text)
    b1 = po.tables["b1"]
    # No 'Year ...' header line -> columns keyed by 0-based index strings.
    assert b1.cells[("1990", "0")] == 10.0
    assert b1.cells[("1990", "2")] == 30.0
