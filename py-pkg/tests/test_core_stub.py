import pytest

import x13cpp
from x13cpp.datasets import load_airline


def test_seasonal_adjust_is_a_stub():
    series = load_airline()
    with pytest.raises(NotImplementedError, match="not yet expose"):
        x13cpp.seasonal_adjust(series)


def test_seasonal_adjust_validates_args_before_the_stub():
    series = load_airline()
    with pytest.raises(TypeError):
        x13cpp.seasonal_adjust(series.values)  # not a TimeSeries
    with pytest.raises(ValueError, match="not both"):
        x13cpp.seasonal_adjust(series, x11=True, seats=True)
