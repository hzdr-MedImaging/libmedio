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

// Tests of the NIfTI-1/NIfTI-2 support. The reference files in data/ were
// written with nibabel (see data/generate_fixtures.py) and are used to check
// the interoperability with an independent NIfTI implementation. Files written
// by libmedio are checked on byte level and by reading them back.

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QtEndian>
#include <QJsonObject>

#include <cstring>
#include <zlib.h>

#include "CMedIODataFactory.h"
#include "CNIFTIFile.h"
#include "CNIFTIMainHeader.h"
#include "CNIFTI1MainHeader.h"
#include "CNIFTI2MainHeader.h"

Q_DECLARE_METATYPE(QSysInfo::Endian)

// NIfTI datatype codes used in the tests
#define DT_UINT8    2
#define DT_INT16    4
#define DT_INT32    8
#define DT_FLOAT32 16
#define DT_FLOAT64 64

// NIFTI_UNITS_MM | NIFTI_UNITS_SEC
#define UNITS_MM_SEC 10

//=============================================================================================
// helpers

// header fields of a NIfTI-1 or NIfTI-2 header in a common representation
struct HeaderInfo {
  qint64 dim[8];
  double pixdim[8];
  int datatype;
  int bitpix;
  double slope;
  double inter;
  int qformCode;
  int sformCode;
  double qoffset[3];
  double srow[3][4];
  int xyztUnits;
  QString descrip;
  QSysInfo::Endian byteOrder;
};

template <class H>
static HeaderInfo headerInfo(const H* h)
{
  HeaderInfo info;

  for(short i=0; i < 8; i++) {
    info.dim[i] = h->dim(i);
    info.pixdim[i] = h->pix_Dim(i);
  }

  info.datatype = h->dataType();
  info.bitpix = h->bit_Pix();
  info.slope = h->scl_Slope();
  info.inter = h->scl_Inter();
  info.qformCode = h->qform_Code();
  info.sformCode = h->sform_Code();
  info.qoffset[0] = h->qoffset_X();
  info.qoffset[1] = h->qoffset_Y();
  info.qoffset[2] = h->qoffset_Z();

  for(short i=0; i < 4; i++) {
    info.srow[0][i] = h->srow_X(i);
    info.srow[1][i] = h->srow_Y(i);
    info.srow[2][i] = h->srow_Z(i);
  }

  info.xyztUnits = h->xyzt_Units();
  info.descrip = QString::fromLatin1(h->descrip(), qstrnlen(h->descrip(), 80));
  info.byteOrder = h->byteOrder();

  return info;
}

static HeaderInfo headerInfo(const CNIFTIMainHeader* h)
{
  if(h->mainHeaderType() == CNIFTIMainHeader::NIFTI1MainHeader)
    return headerInfo(static_cast<const CNIFTI1MainHeader*>(h));

  return headerInfo(static_cast<const CNIFTI2MainHeader*>(h));
}

// set the header fields used by the write tests
template <class H>
static void fillHeader(H* h, const qint64* dims, QSysInfo::Endian byteOrder)
{
  for(short i=0; i < 8; i++) {
    h->setDim(i, dims[i]);
    h->setPix_Dim(i, 1.0);
  }

  h->setPix_Dim(1, 2.0);
  h->setPix_Dim(2, 2.5);
  h->setPix_Dim(3, 3.0);
  h->setDataType(DT_INT16);
  h->setBit_Pix(16);
  h->setScl_Slope(0.5);
  h->setScl_Inter(1.0);
  h->setQform_Code(0);
  h->setSform_Code(1);
  h->setSrow_X(0, -2.0); h->setSrow_X(3,  10.0);
  h->setSrow_Y(1,  2.5); h->setSrow_Y(3, -20.0);
  h->setSrow_Z(2,  3.0); h->setSrow_Z(3, -30.0);
  h->setXyzt_Units(UNITS_MM_SEC);
  h->setDescrip("libmedio write test");
  h->setByteOrder(byteOrder);
}

// voxel value of linear index i in the reference files (see generate_fixtures.py)
static double patternValue(qint64 i, int datatype)
{
  switch(datatype) {
    case DT_FLOAT32:
    case DT_FLOAT64:
      return i * 0.25 - 2.0;

    case DT_UINT8:
      return (i * 7) % 50;
  }

  return (i * 7) % 50 - 10;
}

// voxel value of linear index i of a matrix in machine byte order
static double voxelValue(const char* data, qint64 i, int datatype)
{
  switch(datatype) {
    case DT_UINT8:   return reinterpret_cast<const quint8*>(data)[i];
    case DT_INT16:   return reinterpret_cast<const qint16*>(data)[i];
    case DT_INT32:   return reinterpret_cast<const qint32*>(data)[i];
    case DT_FLOAT32: return reinterpret_cast<const float*>(data)[i];
    case DT_FLOAT64: return reinterpret_cast<const double*>(data)[i];
  }

  return qQNaN();
}

// voxel matrix holding the reference pattern in machine byte order
static QByteArray patternMatrix(qint64 voxels)
{
  QByteArray data(voxels * sizeof(qint16), '\0');
  qint16* p = reinterpret_cast<qint16*>(data.data());

  for(qint64 i=0; i < voxels; i++)
    p[i] = static_cast<qint16>(patternValue(i, DT_INT16));

  return data;
}

// complete (decompressed) content of a file
static QByteArray fileContent(const QString& fileName)
{
  QByteArray content;
  gzFile input = gzopen(QFile::encodeName(fileName).constData(), "rb");

  if(input) {
    char buffer[4096];
    int bytesRead;

    while((bytesRead = gzread(input, buffer, sizeof(buffer))) > 0)
      content.append(buffer, bytesRead);

    gzclose(input);
  }

  return content;
}

template <typename T>
static T fromByteOrder(const char* data, QSysInfo::Endian byteOrder)
{
  return (byteOrder == QSysInfo::BigEndian) ? qFromBigEndian<T>(reinterpret_cast<const uchar*>(data))
                                            : qFromLittleEndian<T>(reinterpret_cast<const uchar*>(data));
}

static bool copyFile(const QString& src, const QString& dst)
{
  QFile::remove(dst);
  return QFile::copy(src, dst) && QFile::setPermissions(dst, QFile::ReadOwner | QFile::WriteOwner);
}

//=============================================================================================
// test class

class TestNIFTIFormat : public QObject
{
  Q_OBJECT

  private slots:
    void testIdentify_data();
    void testIdentify();

    void testReadReference_data();
    void testReadReference();

    void testWriteRoundTrip_data();
    void testWriteRoundTrip();

    void testTrailingBytesIgnored();
    void testTruncatedFileRejected();

    void testMatrixSizeMismatch_data();
    void testMatrixSizeMismatch();

    void testConvertVersion_data();
    void testConvertVersion();
    void testConvertToNIFTI1Limits();

  private:
    QString dataFile(const QString& name) const { return QString(NIFTI_TEST_DATA_DIR) + "/" + name; }
};

//---------------------------------------------------------------------------------------------
void TestNIFTIFormat::testIdentify_data()
{
  QTest::addColumn<QString>("file");
  QTest::addColumn<bool>("isNifti");

  QTest::newRow("NIfTI-1 LE")         << "n1_le_int16.nii"       << true;
  QTest::newRow("NIfTI-1 BE")         << "n1_be_float32_4d.nii"  << true;
  QTest::newRow("NIfTI-1 gzip")       << "n1_le_uint8.nii.gz"    << true;
  QTest::newRow("NIfTI-2 LE")         << "n2_le_float64.nii"     << true;
  QTest::newRow("NIfTI-2 BE")         << "n2_be_int32_ext.nii"   << true;
  QTest::newRow("NIfTI-2 gzip")       << "n2_le_int16_4d.nii.gz" << true;
  QTest::newRow("Analyze 7.5")        << "analyze.hdr"           << false;
  QTest::newRow("NIfTI-1 pair (hdr)") << "n1_pair.hdr"           << false;
  QTest::newRow("NIfTI-1 pair (img)") << "n1_pair.img"           << false;
}

void TestNIFTIFormat::testIdentify()
{
  QFETCH(QString, file);
  QFETCH(bool, isNifti);

  QCOMPARE(CNIFTIFile::isOfType(dataFile(file)), isNifti);
  QCOMPARE(CMedIODataFactory::identify(dataFile(file)) == CMedIOData::NIFTI, isNifti);
}

//---------------------------------------------------------------------------------------------
void TestNIFTIFormat::testReadReference_data()
{
  QTest::addColumn<QString>("file");
  QTest::addColumn<int>("version");
  QTest::addColumn<int>("frames");
  QTest::addColumn<int>("datatype");
  QTest::addColumn<QSysInfo::Endian>("byteOrder");
  QTest::addColumn<bool>("hasJson");

  QTest::newRow("NIfTI-1 LE int16")         << "n1_le_int16.nii"       << 1 << 1 << DT_INT16   << QSysInfo::LittleEndian << false;
  QTest::newRow("NIfTI-1 BE float32 4D")    << "n1_be_float32_4d.nii"  << 1 << 2 << DT_FLOAT32 << QSysInfo::BigEndian    << false;
  QTest::newRow("NIfTI-1 gzip uint8")       << "n1_le_uint8.nii.gz"    << 1 << 1 << DT_UINT8   << QSysInfo::LittleEndian << false;
  QTest::newRow("NIfTI-1 LE extensions")    << "n1_le_int16_ext.nii"   << 1 << 1 << DT_INT16   << QSysInfo::LittleEndian << true;
  QTest::newRow("NIfTI-2 LE float64")       << "n2_le_float64.nii"     << 2 << 1 << DT_FLOAT64 << QSysInfo::LittleEndian << false;
  QTest::newRow("NIfTI-2 BE int32 ext")     << "n2_be_int32_ext.nii"   << 2 << 1 << DT_INT32   << QSysInfo::BigEndian    << true;
  QTest::newRow("NIfTI-2 gzip int16 4D")    << "n2_le_int16_4d.nii.gz" << 2 << 2 << DT_INT16   << QSysInfo::LittleEndian << false;
}

void TestNIFTIFormat::testReadReference()
{
  QFETCH(QString, file);
  QFETCH(int, version);
  QFETCH(int, frames);
  QFETCH(int, datatype);
  QFETCH(QSysInfo::Endian, byteOrder);
  QFETCH(bool, hasJson);

  CNIFTIFile niftiFile(dataFile(file));
  QVERIFY(niftiFile.open(QIODevice::ReadOnly));
  QCOMPARE(static_cast<int>(niftiFile.format()), version == 1 ? static_cast<int>(CNIFTIFile::NIFTI1)
                                                               : static_cast<int>(CNIFTIFile::NIFTI2));

  CNIFTIMainHeader* header = NULL;
  QVERIFY(niftiFile.readMainHeader(header));
  QScopedPointer<CNIFTIMainHeader> headerGuard(header);

  // header fields
  HeaderInfo info = headerInfo(header);
  QCOMPARE(info.dim[0], qint64(frames > 1 ? 4 : 3));
  QCOMPARE(info.dim[1], qint64(4));
  QCOMPARE(info.dim[2], qint64(3));
  QCOMPARE(info.dim[3], qint64(2));
  if(frames > 1)
    QCOMPARE(info.dim[4], qint64(frames));
  QCOMPARE(info.pixdim[1], 2.0);
  QCOMPARE(info.pixdim[2], 2.5);
  QCOMPARE(info.pixdim[3], 3.0);
  QCOMPARE(info.datatype, datatype);
  QCOMPARE(info.slope, 0.5);
  QCOMPARE(info.inter, 1.0);
  QCOMPARE(info.qformCode, 1);
  QCOMPARE(info.sformCode, 1);
  QCOMPARE(info.qoffset[0], 10.0);
  QCOMPARE(info.qoffset[1], -20.0);
  QCOMPARE(info.qoffset[2], -30.0);

  const double affine[3][4] = { { -2.0, 0.0, 0.0,  10.0 },
                                {  0.0, 2.5, 0.0, -20.0 },
                                {  0.0, 0.0, 3.0, -30.0 } };
  for(int r=0; r < 3; r++)
    for(int c=0; c < 4; c++)
      QCOMPARE(info.srow[r][c], affine[r][c]);

  QCOMPARE(info.xyztUnits, UNITS_MM_SEC);
  QCOMPARE(info.descrip, QString("libmedio test fixture"));
  QCOMPARE(info.byteOrder, byteOrder);

  // voxel matrix (returned in machine byte order)
  qint64 voxels = 4 * 3 * 2 * frames;

  QByteArray* matrix = NULL;
  QVERIFY(niftiFile.readMatrix(matrix));
  QScopedPointer<QByteArray> matrixGuard(matrix);
  QCOMPARE(qint64(matrix->size()), voxels * info.bitpix / 8);

  for(qint64 i=0; i < voxels; i++)
    QCOMPARE(voxelValue(matrix->constData(), i, datatype), patternValue(i, datatype));

  char* rawMatrix = NULL;
  unsigned int len = 0;
  QVERIFY(niftiFile.readMatrix(rawMatrix, len));
  QCOMPARE(QByteArray(rawMatrix, len), *matrix);
  delete[] rawMatrix;

  // JSON metadata extension
  QJsonObject json = (version == 1) ? static_cast<CNIFTI1MainHeader*>(header)->readHeaderExtension(niftiFile)
                                    : static_cast<CNIFTI2MainHeader*>(header)->readHeaderExtension(niftiFile);
  if(hasJson) {
    QCOMPARE(json["Isotope_Name"].toString(), QString("F-18"));
    QCOMPARE(json["Dosage"].toDouble(), 300.5);
    QCOMPARE(json["Frame_Duration"].toInt(), 600);
  } else {
    QVERIFY(json.isEmpty());
  }

  niftiFile.close();
}

//---------------------------------------------------------------------------------------------
void TestNIFTIFormat::testWriteRoundTrip_data()
{
  QTest::addColumn<int>("version");
  QTest::addColumn<bool>("compressed");
  QTest::addColumn<QSysInfo::Endian>("byteOrder");
  QTest::addColumn<bool>("withJson");

  for(int version=1; version <= 2; version++) {
    for(int compressed=0; compressed <= 1; compressed++) {
      for(int bigEndian=0; bigEndian <= 1; bigEndian++) {
        for(int withJson=0; withJson <= 1; withJson++) {
          QString name = QString("NIfTI-%1 %2 %3%4").arg(version)
                                                     .arg(compressed ? ".nii.gz" : ".nii")
                                                     .arg(bigEndian ? "BE" : "LE")
                                                     .arg(withJson ? " JSON" : "");
          QTest::newRow(name.toLatin1().constData()) << version << bool(compressed)
                                                     << (bigEndian ? QSysInfo::BigEndian : QSysInfo::LittleEndian)
                                                     << bool(withJson);
        }
      }
    }
  }
}

void TestNIFTIFormat::testWriteRoundTrip()
{
  QFETCH(int, version);
  QFETCH(bool, compressed);
  QFETCH(QSysInfo::Endian, byteOrder);
  QFETCH(bool, withJson);

  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  QString fileName = tmpDir.filePath(compressed ? "test.nii.gz" : "test.nii");

  const qint64 dims[8] = { 4, 5, 4, 3, 2, 1, 1, 1 };
  const qint64 voxels = 5 * 4 * 3 * 2;
  const int headerSize = (version == 1) ? 348 : 540;
  QByteArray matrix = patternMatrix(voxels);

  QJsonObject json;
  json["Isotope_Name"] = "C-11";
  json["Dosage"] = 412.25;

  // write the file
  {
    CNIFTIFile niftiFile(fileName, version == 1 ? CNIFTIMainHeader::NIFTI1 : CNIFTIMainHeader::NIFTI2);
    QVERIFY(niftiFile.open(QIODevice::WriteOnly));

    QScopedPointer<CNIFTIMainHeader> header(niftiFile.createEmptyHeader());
    QVERIFY(header.data() != NULL);

    if(version == 1) {
      CNIFTI1MainHeader* h = static_cast<CNIFTI1MainHeader*>(header.data());
      fillHeader(h, dims, byteOrder);
      if(withJson)
        h->setVox_Offset(headerSize + 4 + h->headerExtensionSize(json));
    } else {
      CNIFTI2MainHeader* h = static_cast<CNIFTI2MainHeader*>(header.data());
      fillHeader(h, dims, byteOrder);
      if(withJson)
        h->setVox_Offset(headerSize + 4 + h->headerExtensionSize(json));
    }

    QVERIFY(niftiFile.writeMainHeader(*header));

    if(withJson) {
      bool written = (version == 1) ? static_cast<CNIFTI1MainHeader*>(header.data())->writeHeaderExtension(niftiFile, json)
                                    : static_cast<CNIFTI2MainHeader*>(header.data())->writeHeaderExtension(niftiFile, json);
      QVERIFY(written);
    }

    QVERIFY(niftiFile.writeMatrix(matrix));
    niftiFile.close();
  }

  // check the file content on byte level
  QByteArray content = fileContent(fileName);
  QVERIFY(content.size() > headerSize + 4);

  QCOMPARE(fromByteOrder<quint32>(content.constData(), byteOrder), quint32(headerSize));

  qint64 voxOffset;
  if(version == 1) {
    QCOMPARE(QByteArray(content.constData()+344, 4), QByteArray("n+1\0", 4));
    voxOffset = static_cast<qint64>(fromByteOrder<float>(content.constData()+108, byteOrder));
  } else {
    QCOMPARE(QByteArray(content.constData()+4, 8), QByteArray("n+2\0\r\n\032\n", 8));
    voxOffset = fromByteOrder<qint64>(content.constData()+168, byteOrder);
  }

  QVERIFY(voxOffset >= headerSize + 4);
  QCOMPARE(voxOffset % 16, qint64(0));
  QCOMPARE(qint64(content.size()), voxOffset + matrix.size());
  QCOMPARE(content.at(headerSize) != 0, withJson); // extension indicator

  // first voxel in the requested byte order
  QCOMPARE(fromByteOrder<qint16>(content.constData()+voxOffset, byteOrder),
           static_cast<qint16>(patternValue(0, DT_INT16)));
  QCOMPARE(fromByteOrder<qint16>(content.constData()+voxOffset+2, byteOrder),
           static_cast<qint16>(patternValue(1, DT_INT16)));

  // read the file back
  CNIFTIFile niftiFile(fileName);
  QVERIFY(niftiFile.open(QIODevice::ReadOnly));

  CNIFTIMainHeader* header = NULL;
  QVERIFY(niftiFile.readMainHeader(header));
  QScopedPointer<CNIFTIMainHeader> headerGuard(header);

  HeaderInfo info = headerInfo(header);
  for(int i=0; i < 5; i++)
    QCOMPARE(info.dim[i], dims[i]);
  QCOMPARE(info.datatype, DT_INT16);
  QCOMPARE(info.bitpix, 16);
  QCOMPARE(info.slope, 0.5);
  QCOMPARE(info.inter, 1.0);
  QCOMPARE(info.sformCode, 1);
  QCOMPARE(info.srow[0][0], -2.0);
  QCOMPARE(info.srow[1][3], -20.0);
  QCOMPARE(info.srow[2][2], 3.0);
  QCOMPARE(info.xyztUnits, UNITS_MM_SEC);
  QCOMPARE(info.descrip, QString("libmedio write test"));
  QCOMPARE(info.byteOrder, byteOrder);

  QByteArray* readMatrix = NULL;
  QVERIFY(niftiFile.readMatrix(readMatrix));
  QScopedPointer<QByteArray> matrixGuard(readMatrix);
  QCOMPARE(*readMatrix, matrix);

  QJsonObject readJson = (version == 1) ? static_cast<CNIFTI1MainHeader*>(header)->readHeaderExtension(niftiFile)
                                        : static_cast<CNIFTI2MainHeader*>(header)->readHeaderExtension(niftiFile);
  QCOMPARE(readJson, withJson ? json : QJsonObject());

  niftiFile.close();
}

//---------------------------------------------------------------------------------------------
void TestNIFTIFormat::testTrailingBytesIgnored()
{
  QTemporaryDir tmpDir;
  QString fileName = tmpDir.filePath("trailing.nii");
  QVERIFY(copyFile(dataFile("n1_le_int16.nii"), fileName));

  QFile file(fileName);
  QVERIFY(file.open(QIODevice::Append));
  file.write("trailing bytes after the voxel matrix");
  file.close();

  CNIFTIFile niftiFile(fileName);
  QVERIFY(niftiFile.open(QIODevice::ReadOnly));

  QByteArray* matrix = NULL;
  QVERIFY(niftiFile.readMatrix(matrix));
  QCOMPARE(matrix->size(), 4 * 3 * 2 * 2);
  QCOMPARE(voxelValue(matrix->constData(), 23, DT_INT16), patternValue(23, DT_INT16));
  delete matrix;

  niftiFile.close();
}

void TestNIFTIFormat::testTruncatedFileRejected()
{
  QTemporaryDir tmpDir;
  QString fileName = tmpDir.filePath("truncated.nii");
  QVERIFY(copyFile(dataFile("n1_le_int16.nii"), fileName));

  QFile file(fileName);
  QVERIFY(file.resize(file.size() - 2));

  CNIFTIFile niftiFile(fileName);
  QVERIFY(niftiFile.open(QIODevice::ReadOnly));

  QByteArray* matrix = NULL;
  QVERIFY(niftiFile.readMatrix(matrix) == false);
  QVERIFY(matrix == NULL);

  niftiFile.close();
}

//---------------------------------------------------------------------------------------------
void TestNIFTIFormat::testMatrixSizeMismatch_data()
{
  QTest::addColumn<int>("version");

  QTest::newRow("NIfTI-1") << 1;
  QTest::newRow("NIfTI-2") << 2;
}

void TestNIFTIFormat::testMatrixSizeMismatch()
{
  QFETCH(int, version);

  QTemporaryDir tmpDir;
  QString fileName = tmpDir.filePath("mismatch.nii");

  // 4D header with two frames, but only the data of one frame
  const qint64 dims[8] = { 4, 5, 4, 3, 2, 1, 1, 1 };

  CNIFTIFile niftiFile(fileName, version == 1 ? CNIFTIMainHeader::NIFTI1 : CNIFTIMainHeader::NIFTI2);
  QVERIFY(niftiFile.open(QIODevice::WriteOnly));

  QScopedPointer<CNIFTIMainHeader> header(niftiFile.createEmptyHeader());
  if(version == 1)
    fillHeader(static_cast<CNIFTI1MainHeader*>(header.data()), dims, QSysInfo::LittleEndian);
  else
    fillHeader(static_cast<CNIFTI2MainHeader*>(header.data()), dims, QSysInfo::LittleEndian);

  QVERIFY(niftiFile.writeMainHeader(*header));
  QVERIFY(niftiFile.writeMatrix(patternMatrix(5 * 4 * 3)) == false);
  QVERIFY(niftiFile.writeMatrix(patternMatrix(5 * 4 * 3 * 3)) == false);

  niftiFile.close();

  // no voxel data must have been written
  QVERIFY(QFileInfo(fileName).size() <= (version == 1 ? 348 : 540) + 4);
}

//---------------------------------------------------------------------------------------------
void TestNIFTIFormat::testConvertVersion_data()
{
  QTest::addColumn<QString>("file");
  QTest::addColumn<int>("targetVersion");

  QTest::newRow("NIfTI-1 LE -> NIfTI-1") << "n1_le_int16.nii"       << 1;
  QTest::newRow("NIfTI-1 LE -> NIfTI-2") << "n1_le_int16.nii"       << 2;
  QTest::newRow("NIfTI-1 BE -> NIfTI-2") << "n1_be_float32_4d.nii"  << 2;
  QTest::newRow("NIfTI-2 LE -> NIfTI-1") << "n2_le_float64.nii"     << 1;
  QTest::newRow("NIfTI-2 BE -> NIfTI-1") << "n2_be_int32_ext.nii"   << 1;
  QTest::newRow("NIfTI-2 gz -> NIfTI-2") << "n2_le_int16_4d.nii.gz" << 2;
}

void TestNIFTIFormat::testConvertVersion()
{
  QFETCH(QString, file);
  QFETCH(int, targetVersion);

  QTemporaryDir tmpDir;
  QString fileName = tmpDir.filePath("converted.nii");

  CNIFTIFile srcFile(dataFile(file));
  QVERIFY(srcFile.open(QIODevice::ReadOnly));

  QByteArray* srcMatrix = NULL;
  QVERIFY(srcFile.readMatrix(srcMatrix));
  QScopedPointer<QByteArray> srcMatrixGuard(srcMatrix);

  CNIFTIMainHeader* srcHeader = NULL;
  QVERIFY(srcFile.readMainHeader(srcHeader));
  QScopedPointer<CNIFTIMainHeader> srcHeaderGuard(srcHeader);
  HeaderInfo srcInfo = headerInfo(srcHeader);

  // convert by means of the generic CMedIOHeader interface
  {
    CNIFTIFile dstFile(fileName, targetVersion == 1 ? CNIFTIMainHeader::NIFTI1 : CNIFTIMainHeader::NIFTI2);
    QVERIFY(dstFile.open(QIODevice::WriteOnly));

    QScopedPointer<CNIFTIMainHeader> dstHeader(dstFile.createEmptyHeader());
    QVERIFY(static_cast<CMedIOHeader*>(dstHeader.data())->convertFrom(&srcFile));
    QVERIFY(dstFile.writeMainHeader(*dstHeader));
    QVERIFY(dstFile.writeMatrix(*srcMatrix));
    dstFile.close();
  }
  srcFile.close();

  CNIFTIFile dstFile(fileName);
  QVERIFY(dstFile.open(QIODevice::ReadOnly));
  QCOMPARE(static_cast<int>(dstFile.format()), targetVersion == 1 ? static_cast<int>(CNIFTIFile::NIFTI1)
                                                                   : static_cast<int>(CNIFTIFile::NIFTI2));

  CNIFTIMainHeader* dstHeader = NULL;
  QVERIFY(dstFile.readMainHeader(dstHeader));
  QScopedPointer<CNIFTIMainHeader> dstHeaderGuard(dstHeader);
  HeaderInfo dstInfo = headerInfo(dstHeader);

  for(int i=0; i < 8; i++) {
    QCOMPARE(dstInfo.dim[i], srcInfo.dim[i]);
    QCOMPARE(dstInfo.pixdim[i], srcInfo.pixdim[i]);
  }
  QCOMPARE(dstInfo.datatype, srcInfo.datatype);
  QCOMPARE(dstInfo.bitpix, srcInfo.bitpix);
  QCOMPARE(dstInfo.slope, srcInfo.slope);
  QCOMPARE(dstInfo.inter, srcInfo.inter);
  QCOMPARE(dstInfo.qformCode, srcInfo.qformCode);
  QCOMPARE(dstInfo.sformCode, srcInfo.sformCode);
  for(int r=0; r < 3; r++) {
    QCOMPARE(dstInfo.qoffset[r], srcInfo.qoffset[r]);
    for(int c=0; c < 4; c++)
      QCOMPARE(dstInfo.srow[r][c], srcInfo.srow[r][c]);
  }
  QCOMPARE(dstInfo.xyztUnits, srcInfo.xyztUnits);
  QCOMPARE(dstInfo.descrip, srcInfo.descrip);

  // converted files are always written in little endian byte order
  QCOMPARE(dstInfo.byteOrder, QSysInfo::LittleEndian);

  QByteArray* dstMatrix = NULL;
  QVERIFY(dstFile.readMatrix(dstMatrix));
  QCOMPARE(*dstMatrix, *srcMatrix);
  delete dstMatrix;

  dstFile.close();
}

void TestNIFTIFormat::testConvertToNIFTI1Limits()
{
  CNIFTI2MainHeader nifti2;
  nifti2.setDim(0, 3);
  nifti2.setDim(1, 300);
  nifti2.setDim(2, 300);
  nifti2.setDim(3, 1);

  CNIFTI1MainHeader nifti1;
  QVERIFY(nifti1.convertFrom(&nifti2));
  QCOMPARE(nifti1.dim(1), short(300));

  // NIfTI-1 only supports 16 bit dimensions
  nifti2.setDim(1, 40000);
  QVERIFY(nifti1.convertFrom(&nifti2) == false);
}

QTEST_GUILESS_MAIN(TestNIFTIFormat)
#include "testNIFTIFormat.moc"
