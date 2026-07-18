import os

from x13compare import parse_save, parse_udg

FIXTURES = os.path.join(os.path.dirname(__file__), "fixtures")


def test_save_parses_label_and_values():
    ps = parse_save.parse_save_file(os.path.join(FIXTURES, "sample.d11"))
    assert ps.label == "SALES.d11"
    assert len(ps) == 8
    assert ps["199001"] == 100.0
    assert ps["199008"] == 107.0
    # header lines are captured, not counted as data
    assert any(h.lower().startswith("date") for h in ps.header_lines)


def test_save_skips_both_header_lines():
    text = "date\tX.d11\n------\t-----\n199001\t1.5\n199002\t2.5\n"
    ps = parse_save.parse_save(text)
    assert set(ps.keys()) == {"199001", "199002"}
    assert ps["199002"] == 2.5


def test_save_annual_dates():
    text = "date\tX.tot\n------\t-----\n1990\t12.0\n1991\t13.0\n"
    ps = parse_save.parse_save(text)
    assert ps["1990"] == 12.0
    assert ps["1991"] == 13.0


def test_save_fortran_d_exponent():
    text = "date\tX.d11\n------\t-----\n199001\t1.5D+02\n"
    ps = parse_save.parse_save(text)
    assert ps["199001"] == 150.0


def test_udg_auto_typing():
    pu = parse_udg.parse_udg_file(os.path.join(FIXTURES, "sample.udg"))
    assert pu["version"] == "1.1 build 61"      # string
    assert pu["nobs"] == 24.0                     # single float
    assert pu["aic"] == 1234.56789                # single float
    assert pu["seatsmdl"] == [0.0, 1.0, 1.0, 0.0, 1.0, 1.0]  # float list


def test_udg_colon_in_value():
    pu = parse_udg.parse_udg("label: a:b:c\n")
    assert pu["label"] == "a:b:c"


def test_udg_empty_value():
    pu = parse_udg.parse_udg("emptykey:\n")
    assert pu["emptykey"] == ""
