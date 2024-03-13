import pmedio
import pytest

def test_constructors():
  a = pmedio.MedIOImage(1,1,1,1)
  assert a.ndim == 4
  assert a.shape == (1,1,1,1)
  a[0,0,0,0] = 123.456
  assert a[0,0,0,0] == pytest.approx(123.456, 3)
