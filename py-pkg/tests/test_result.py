import os

import pytest

from x13cpp._result import X13Result
from x13cpp._timeseries import TimeSeries
from x13cpp.datasets import load_airline


def make_fake_result():
    s = load_airline()
    fct = TimeSeries(values=[float("nan")] * 12, start=(1961, 1), freq=12, name="fct")
    return X13Result(
        call="seasonal_adjust(airline)",
        series=s,
        udg={"nobs": len(s), "aicc": 123.45},
        fct=fct,
        save={"d11": s, "d12": s, "d13": s},
        status="ok",
    )


def test_accessors():
    res = make_fake_result()
    assert res.get_udg() == {"nobs": 144, "aicc": 123.45}
    assert res.get_fct() is not None
    assert res.seasadj() is res.save["d11"]
    assert res.trend() is res.save["d12"]
    assert res.irregular() is res.save["d13"]
    assert res.save_table("d11") is res.save["d11"]


def test_save_table_unknown_raises():
    res = make_fake_result()
    with pytest.raises(KeyError, match="not present"):
        res.save_table("s10")


def test_write_outputs_only_on_explicit_call(tmp_path):
    res = make_fake_result()
    out = str(tmp_path)

    assert not os.path.exists(os.path.join(out, "series.udg"))
    res.write_outputs(out)
    assert os.path.exists(os.path.join(out, "series.udg"))
    assert os.path.exists(os.path.join(out, "series.fct"))
    assert os.path.exists(os.path.join(out, "series.d11"))

    with pytest.raises(FileExistsError):
        res.write_outputs(out)

    # overwrite=True should succeed
    res.write_outputs(out, overwrite=True)
