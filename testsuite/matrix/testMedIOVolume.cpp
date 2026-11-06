/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio

  Copyright (C) 2004-2026 hzdr.de and contributors

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

// Tests of CMedIOVolume: values, scaling and reorientation. The expected
// voxel of every reoriented voxel is computed in the test from the original
// affine (inverse transformation of the voxel position).

#include <CMedIOVolume.h>

#include <QtTest/QtTest>

#include <cmath>

// 4 x 3 x 2 int16 volume with stored value = 1 + linear index
static CMedIOVolume makeVolume(const double m[3][4])
{
  QByteArray data(24 * 2, '\0');
  for(int i=0; i < 24; i++)
    reinterpret_cast<qint16*>(data.data())[i] = static_cast<qint16>(i + 1);

  CMedIOVolume v;
  v.setData(4, 3, 2, CMedIOVolume::Int16, data);
  v.setAffine(m);
  return v;
}

// voxel (possibly fractional) of the original volume at LPS position p
static void inverse(const double a[3][4], const double p[3], double v[3])
{
  const double q[3] = { p[0] - a[0][3], p[1] - a[1][3], p[2] - a[2][3] };
  const double det = a[0][0]*(a[1][1]*a[2][2]-a[1][2]*a[2][1]) - a[0][1]*(a[1][0]*a[2][2]-a[1][2]*a[2][0]) + a[0][2]*(a[1][0]*a[2][1]-a[1][1]*a[2][0]);
  for(int i=0; i < 3; i++) {
    v[i] = 0.0;
    for(int j=0; j < 3; j++) {
      const int i1 = (j + 1) % 3, i2 = (j + 2) % 3, j1 = (i + 1) % 3, j2 = (i + 2) % 3;
      v[i] += (a[i1][j1] * a[i2][j2] - a[i1][j2] * a[i2][j1]) / det * q[j];
    }
  }
}

class TestMedIOVolume : public QObject
{
  Q_OBJECT

  private slots:
    void values();
    void reorient_data();
    void reorient();
    void planeScales();
    void errors();
};

void TestMedIOVolume::values()
{
  const double m[3][4] = { { 2, 0, 0, 10 }, { 0, 2.5, 0, -20 }, { 0, 0, 3, -30 } };
  CMedIOVolume v = makeVolume(m);

  QCOMPARE(v.voxelCount(), static_cast<qint64>(24));
  QCOMPARE(v.dimension(0), 4); QCOMPARE(v.dimension(1), 3); QCOMPARE(v.dimension(2), 2);
  QCOMPARE(v.voxelSize(1), 2.5);
  QCOMPARE(v.orientation(), QString("LPS"));

  v.setScaleFactor(0.5);
  v.setScaleFactor(1, 2.0);
  v.setIntercept(1.0);
  QVERIFY(v.hasUniformScaleFactor() == false);
  QCOMPARE(v.storedValue(13), 14.0);
  QCOMPARE(v.value(0), 1.5);        // plane 0: 1 x 0.5 + 1
  QCOMPARE(v.value(13), 29.0);      // plane 1: 14 x 2 + 1

  CMedIOVolume copy(v);
  copy.convertToPhysicalValues();
  QCOMPARE(copy.dataType(), CMedIOVolume::Float32);
  QCOMPARE(copy.scaleFactor(1), 1.0);
  QCOMPARE(copy.intercept(), 0.0);
  for(qint64 i=0; i < 24; i++)
    QCOMPARE(copy.value(i), v.value(i));
  QCOMPARE(v.dataType(), CMedIOVolume::Int16); // copies are independent

  double p[3];
  v.position(1, 2, 1, p);
  QCOMPARE(p[0], 12.0); QCOMPARE(p[1], -15.0); QCOMPARE(p[2], -27.0);
}

void TestMedIOVolume::reorient_data()
{
  QTest::addColumn<QVector<double> >("affine");
  QTest::addColumn<QString>("source");   // orientation code of the affine
  QTest::addColumn<QString>("target");
  QTest::addColumn<bool>("oblique");

  const QVector<double> lps = QVector<double>() << 2 << 0 << 0 << 10 << 0 << 2.5 << 0 << -20 << 0 << 0 << 3 << -30;
  const QVector<double> ras = QVector<double>() << -2 << 0 << 0 << 10 << 0 << -2.5 << 0 << -20 << 0 << 0 << 3 << -30;
  const QVector<double> permuted = QVector<double>() << 0 << 0 << -3 << 10 << 2 << 0 << 0 << -20 << 0 << 2.5 << 0 << -30;
  const QVector<double> oblique = QVector<double>() << 1.98 << 0.2 << 0 << 10 << -0.2 << 2.48 << 0 << -20 << 0 << 0 << 3 << -30;

  QTest::newRow("LPS -> LPI (ECAT7)")  << lps << "LPS" << "LPI" << false;
  QTest::newRow("LPS -> RAS")          << lps << "LPS" << "RAS" << false;
  QTest::newRow("RAS -> LPI")          << ras << "RAS" << "LPI" << false;
  QTest::newRow("RAS -> RAS")          << ras << "RAS" << "RAS" << false;
  QTest::newRow("PSR -> LPI")          << permuted << "PSR" << "LPI" << false;
  QTest::newRow("PSR -> SRA")          << permuted << "PSR" << "SRA" << false;
  QTest::newRow("oblique -> LPI")      << oblique << "LPS" << "LPI" << true;
  QTest::newRow("lower case")          << lps << "LPS" << "rai" << false;
}

void TestMedIOVolume::reorient()
{
  QFETCH(QVector<double>, affine);
  QFETCH(QString, source);
  QFETCH(QString, target);
  QFETCH(bool, oblique);

  double a[3][4];
  for(int i=0; i < 12; i++)
    a[i / 4][i % 4] = affine[i];

  CMedIOVolume v = makeVolume(a);
  v.setFrameStart(10.0);
  QCOMPARE(v.orientation(), source);

  bool isOblique = !oblique;
  QVERIFY(v.reorient(target, &isOblique));
  QCOMPARE(isOblique, oblique);
  QCOMPARE(v.orientation(), target.toUpper());
  QCOMPARE(v.voxelCount(), static_cast<qint64>(24));
  QCOMPARE(v.frameStart(), 10.0);

  // every voxel keeps its position: value = 1 + index of the original voxel at that position
  for(int k=0; k < v.dimension(2); k++)
    for(int j=0; j < v.dimension(1); j++)
      for(int i=0; i < v.dimension(0); i++) {
        double p[3], o[3];
        v.position(i, j, k, p);
        inverse(a, p, o);
        for(int r=0; r < 3; r++)
          QVERIFY2(std::fabs(o[r] - qRound(o[r])) < 1e-6, qPrintable(QString("voxel (%1,%2,%3)").arg(i).arg(j).arg(k)));
        const qint64 index = i + v.dimension(0) * (j + static_cast<qint64>(v.dimension(1)) * k);
        QCOMPARE(v.value(index), 1.0 + qRound(o[0]) + 4 * (qRound(o[1]) + 3 * qRound(o[2])));
      }

  // a volume without data changes its geometry only
  CMedIOVolume empty;
  empty.setAffine(a);
  QVERIFY(empty.reorient(target));
  QCOMPARE(empty.orientation(), target.toUpper());
}

void TestMedIOVolume::planeScales()
{
  // reversed planes keep their scale factors
  const double lps[3][4] = { { 2, 0, 0, 0 }, { 0, 2, 0, 0 }, { 0, 0, 3, 0 } };
  CMedIOVolume v = makeVolume(lps);
  v.setScaleFactor(0, 0.5);
  v.setScaleFactor(1, 2.0);
  QVERIFY(v.reorient("LPI"));
  QCOMPARE(v.dataType(), CMedIOVolume::Int16);
  QCOMPARE(v.scaleFactor(0), 2.0);
  QCOMPARE(v.scaleFactor(1), 0.5);
  QCOMPARE(v.value(0), 13 * 2.0);

  // planes becoming rows: physical values
  CMedIOVolume w = makeVolume(lps);
  w.setScaleFactor(0, 0.5);
  w.setScaleFactor(1, 2.0);
  QVERIFY(w.reorient("SPL"));
  QCOMPARE(w.dataType(), CMedIOVolume::Float32);
  QVERIFY(w.hasUniformScaleFactor());
  QCOMPARE(w.dimension(0), 2);
  QCOMPARE(w.value(1), 13 * 2.0); // voxel (1, 0, 0) = original (0, 0, 1)
}

void TestMedIOVolume::errors()
{
  const double lps[3][4] = { { 2, 0, 0, 0 }, { 0, 2, 0, 0 }, { 0, 0, 3, 0 } };
  CMedIOVolume v = makeVolume(lps);
  QVERIFY(v.reorient("LP") == false);
  QVERIFY(v.reorient("LPX") == false);
  QVERIFY(v.reorient("LRS") == false);   // two letters for x
  QCOMPARE(v.orientation(), QString("LPS"));

  const double degenerate[3][4] = { { 2, 2, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 3, 0 } };
  CMedIOVolume d = makeVolume(degenerate);
  QVERIFY(d.reorient("LPI") == false);

  CMedIOVolume bad;
  QVERIFY(bad.setData(4, 3, 2, CMedIOVolume::Int16, QByteArray(10, '\0')) == false);
  QVERIFY(bad.setData(0, 3, 2, CMedIOVolume::Int16, QByteArray()) == false);
  QVERIFY(bad.setData(1, 1, 1, CMedIOVolume::UnknownDataType, QByteArray(1, '\0')) == false);
  QVERIFY(bad.isEmpty());
}

QTEST_GUILESS_MAIN(TestMedIOVolume)
#include "testMedIOVolume.moc"
