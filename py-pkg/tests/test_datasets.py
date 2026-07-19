import pytest

from x13cpp.datasets import (
    load_airline,
    load_expgs,
    load_payems,
    load_retail_sales,
    load_shoe_sales,
    load_unrate,
)


def test_load_airline():
    s = load_airline()
    assert len(s) == 144
    assert s.start == (1949, 1)
    assert s.freq == 12
    assert s.values[0] == 112.0


def test_load_expgs():
    s = load_expgs()
    assert len(s) == 317
    assert s.start == (1947, 1)
    assert s.freq == 4


def test_load_payems():
    s = load_payems()
    assert len(s) == 308
    assert s.start == (2000, 1)
    assert s.freq == 12


def test_load_unrate():
    s = load_unrate()
    assert len(s) == 932
    assert s.start == (1948, 1)
    assert s.freq == 12
    assert s.values[0] == pytest.approx(3.4)


def test_placeholder_loaders_raise():
    with pytest.raises(NotImplementedError):
        load_shoe_sales()
    with pytest.raises(NotImplementedError):
        load_retail_sales()
