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

// Validates the NIfTI files of the test datasets (testsuite/data/nifti, single
// files and header/image pair directories) against their expected values
// <file>.json, which are generated with nibabel (see testsuite/data/README.md),
// and checks that no other file of the test data is identified as NIfTI.

#include <CNIFTIFile.h>
#include <CNIFTIMainHeader.h>
#include <CNIFTI1MainHeader.h>
#include <CNIFTI2MainHeader.h>
#include <CMedIODataFactory.h>

#include "MedIOTestData.h"

#include <QtTest/QtTest>
#include <QDir>
#include <QJsonArray>

#include <cmath>

// header values compared with the expected values
struct NiftiHeader
{
  qint64 dim[8];
  double pixdim[8];
  int datatype, bitpix;
  double slope, inter;
  int qformCode, sformCode;
  double quatern[3], qoffset[3], srow[3][4];
  int xyztUnits;
  QSysInfo::Endian byteOrder;
};

template <class H>
static NiftiHeader niftiHeader(const H* h)
{
  NiftiHeader n;
  for(short i=0; i < 8; i++) {
    n.dim[i] = h->dim(i);
    n.pixdim[i] = h->pix_Dim(i);
  }
  n.datatype = h->dataType();
  n.bitpix = h->bit_Pix();
  n.slope = h->scl_Slope();
  n.inter = h->scl_Inter();
  n.qformCode = h->qform_Code();
  n.sformCode = h->sform_Code();
  n.quatern[0] = h->quatern_B(); n.quatern[1] = h->quatern_C(); n.quatern[2] = h->quatern_D();
  n.qoffset[0] = h->qoffset_X(); n.qoffset[1] = h->qoffset_Y(); n.qoffset[2] = h->qoffset_Z();
  for(short i=0; i < 4; i++) {
    n.srow[0][i] = h->srow_X(i);
    n.srow[1][i] = h->srow_Y(i);
    n.srow[2][i] = h->srow_Z(i);
  }
  n.xyztUnits = static_cast<int>(h->xyzt_Units());
  n.byteOrder = h->byteOrder();
  return n;
}

// stored value of voxel i (machine byte order)
static double storedValue(const char* data, qint64 i, int datatype, bool* ok)
{
  *ok = true;
  switch(datatype) {
    case 2:   return reinterpret_cast<const quint8*>(data)[i];
    case 256: return reinterpret_cast<const qint8*>(data)[i];
    case 4:   return reinterpret_cast<const qint16*>(data)[i];
    case 512: return reinterpret_cast<const quint16*>(data)[i];
    case 8:   return reinterpret_cast<const qint32*>(data)[i];
    case 768: return reinterpret_cast<const quint32*>(data)[i];
    case 16:  return reinterpret_cast<const float*>(data)[i];
    case 64:  return reinterpret_cast<const double*>(data)[i];
  }
  *ok = false;
  return 0.0;
}

static bool sameValue(double value, double expected, double relTol)
{
  return std::fabs(value - expected) <= relTol * qMax(1.0, std::fabs(expected));
}

// the file to open for a test data entry: the .hdr file of a pair directory
static QString niftiPath(const MedIOTestFile& file)
{
  if(QFileInfo(file.path).isDir() == false)
    return file.path;

  const QStringList headers = QDir(file.path).entryList(QStringList() << "*.hdr" << "*.HDR" << "*.hdr.gz", QDir::Files);
  return headers.size() == 1 ? QDir(file.path).filePath(headers.first()) : QString();
}

class TestNIFTIDatasets : public QObject
{
  Q_OBJECT

  private:
    MedIOTestData m_data;

  private slots:
    void initTestCase();

    void niftiFiles_data();
    void niftiFiles();

    void otherFiles_data();
    void otherFiles();
};

void TestNIFTIDatasets::initTestCase()
{
  m_data = medioTestData();
  foreach(const QString& pending, m_data.pending)
    qInfo("pending test data file: %s", qPrintable(pending));
}

//---------------------------------------------------------------------------------------------
// NIfTI files: header and stored voxel values

void TestNIFTIDatasets::niftiFiles_data()
{
  QTest::addColumn<int>("index");

  bool any = false;
  for(int i=0; i < m_data.files.size(); i++) {
    if(m_data.files[i].format == "nifti") {
      QTest::newRow(qPrintable(m_data.files[i].relativePath)) << i;
      any = true;
    }
  }

  if(any == false)
    QTest::newRow("no NIfTI test data") << -1;
}

void TestNIFTIDatasets::niftiFiles()
{
  QFETCH(int, index);
  if(index < 0)
    QSKIP("no NIfTI files in the test data");

  const MedIOTestFile& file = m_data.files[index];
  const QJsonObject expected = file.expected;
  const QString format = expected.value("format").toString();
  const QString path = niftiPath(file);
  QVERIFY2(path.isEmpty() == false, "a NIfTI pair directory has to contain exactly one .hdr file");

  QVERIFY(CNIFTIFile::isOfType(path));
  QCOMPARE(CMedIODataFactory::identify(path), CMedIOData::NIFTI);

  CNIFTIFile niftiFile(path);
  QVERIFY(niftiFile.open(QIODevice::ReadOnly));

  // format
  const bool pair = format.endsWith("-pair");
  QCOMPARE(niftiFile.isPair(), pair);
  QCOMPARE(static_cast<int>(niftiFile.format()), format.startsWith("nifti2") ? int(CNIFTIFile::NIFTI2) : int(CNIFTIFile::NIFTI1));

  CNIFTIMainHeader* header = NULL;
  QVERIFY(niftiFile.readMainHeader(header));
  QScopedPointer<CNIFTIMainHeader> guard(header);

  const NiftiHeader n = (header->mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader)
                        ? niftiHeader(static_cast<const CNIFTI1MainHeader*>(header))
                        : niftiHeader(static_cast<const CNIFTI2MainHeader*>(header));

  // header values
  const QJsonObject h = expected.value("header").toObject();
  const QJsonArray dim = h.value("dim").toArray();
  const QJsonArray pixdim = h.value("pixdim").toArray();
  QVERIFY(dim.size() > 0);
  QCOMPARE(n.dim[0], qint64(dim[0].toInt()));
  for(int i=1; i < dim.size(); i++)
    QCOMPARE(n.dim[i], qint64(dim[i].toInt()));
  for(int i=0; i < pixdim.size(); i++) {
    // pixdim[0] is qfac: nibabel (and the NIfTI standard) treat 0 as 1
    const double value = (i == 0) ? (n.pixdim[0] < 0 ? -1.0 : 1.0) : n.pixdim[i];
    QVERIFY2(sameValue(value, pixdim[i].toDouble(), 1e-6), qPrintable(QString("pixdim[%1]").arg(i)));
  }

  QCOMPARE(n.datatype, h.value("datatype").toInt());
  QCOMPARE(n.bitpix, h.value("bitpix").toInt());
  QCOMPARE(n.qformCode, h.value("qform_code").toInt());
  QCOMPARE(n.sformCode, h.value("sform_code").toInt());
  QCOMPARE(n.xyztUnits, h.value("xyzt_units").toInt());
  QCOMPARE(n.byteOrder, h.value("byte_order").toString() == "big" ? QSysInfo::BigEndian : QSysInfo::LittleEndian);

  // nibabel reports no scaling (scl_slope 0 or not finite) as slope 1
  double slope = n.slope, inter = n.inter;
  if(slope == 0.0 || std::isfinite(slope) == false) {
    slope = 1.0;
    inter = 0.0;
  }
  QVERIFY(sameValue(slope, h.value("scl_slope").toDouble(), 1e-6));
  QVERIFY(sameValue(inter, h.value("scl_inter").toDouble(), 1e-6));

  const QJsonArray quatern = h.value("quatern").toArray();
  const QJsonArray qoffset = h.value("qoffset").toArray();
  const QJsonArray srow = h.value("srow").toArray();
  for(int i=0; i < 3; i++) {
    QVERIFY(sameValue(n.quatern[i], quatern[i].toDouble(), 1e-6));
    QVERIFY(sameValue(n.qoffset[i], qoffset[i].toDouble(), 1e-6));
    for(int j=0; j < 4; j++)
      QVERIFY(sameValue(n.srow[i][j], srow[i].toArray()[j].toDouble(), 1e-6));
  }

  // stored voxel values per 3D volume
  QByteArray* matrix = NULL;
  QVERIFY(niftiFile.readMatrix(matrix));
  QScopedPointer<QByteArray> matrixGuard(matrix);

  const QJsonArray volumes = expected.value("volumes").toArray();
  const qint64 volume = n.dim[1] * qMax(qint64(1), n.dim[2]) * qMax(qint64(1), n.dim[3]);
  QCOMPARE(qint64(matrix->size()), volume * volumes.size() * n.bitpix / 8);

  const bool integer = (n.datatype != 16 && n.datatype != 64);
  for(int v=0; v < volumes.size(); v++) {
    double minValue = 0.0, maxValue = 0.0, sum = 0.0;
    for(qint64 i=0; i < volume; i++) {
      bool ok = false;
      const double value = storedValue(matrix->constData(), v * volume + i, n.datatype, &ok);
      QVERIFY2(ok, "unsupported datatype");
      minValue = (i == 0) ? value : qMin(minValue, value);
      maxValue = (i == 0) ? value : qMax(maxValue, value);
      sum += value;
    }

    const QJsonObject e = volumes[v].toObject();
    const double tol = integer ? 0.0 : 1e-6;
    QVERIFY2(sameValue(minValue, e.value("raw_min").toDouble(), tol), qPrintable(QString("volume %1 min %2").arg(v).arg(minValue)));
    QVERIFY2(sameValue(maxValue, e.value("raw_max").toDouble(), tol), qPrintable(QString("volume %1 max %2").arg(v).arg(maxValue)));
    QVERIFY2(sameValue(sum, e.value("raw_sum").toDouble(), integer ? 0.0 : 1e-5),
             qPrintable(QString("volume %1 sum %2").arg(v).arg(sum, 0, 'g', 15)));
  }

  niftiFile.close();
}

//---------------------------------------------------------------------------------------------
// the files of other formats (e.g. Philips .img, Interfile .hdr) are no NIfTI files

void TestNIFTIDatasets::otherFiles_data()
{
  QTest::addColumn<QString>("path");

  bool any = false;
  foreach(const MedIOTestFile& file, m_data.files) {
    if(file.format == "nifti")
      continue;

    QStringList paths;
    if(QFileInfo(file.path).isDir()) {
      foreach(const QString& name, QDir(file.path).entryList(QDir::Files))
        paths << QDir(file.path).filePath(name);
    } else {
      paths << file.path;
    }

    // DICOM series: one slice is sufficient
    if(file.format == "dicom" && paths.size() > 1)
      paths = paths.mid(0, 1);

    foreach(const QString& path, paths) {
      QTest::newRow(qPrintable(QDir(medioTestDataDir()).relativeFilePath(path))) << path;
      any = true;
    }
  }

  if(any == false)
    QTest::newRow("no other test data") << QString();
}

void TestNIFTIDatasets::otherFiles()
{
  QFETCH(QString, path);
  if(path.isEmpty())
    QSKIP("no files of other formats in the test data");

  QVERIFY(CNIFTIFile::isOfType(path) == false);
  QVERIFY(CMedIODataFactory::identify(path) != CMedIOData::NIFTI);
}

QTEST_GUILESS_MAIN(TestNIFTIDatasets)
#include "testNIFTIDatasets.moc"
