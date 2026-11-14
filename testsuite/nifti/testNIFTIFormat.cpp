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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <cmath>
#include <cstring>
#include <zlib.h>

#include "CMedIODataFactory.h"
#include "CMedIOHeaderMapping.h"
#include "CMedIOVolume.h"
#include "CECATFile.h"
#include "CECAT7MainHeader.h"
#include "CECAT7SubHeaderImage.h"
#include "CNIFTIConverter.h"
#include "CNIFTIExtensions.h"
#include "CNIFTIFile.h"
#include "CNIFTIMainHeader.h"
#include "CNIFTI1MainHeader.h"
#include "CNIFTI2MainHeader.h"
#include "MedIOTestTimeZone.h"

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
    void testPairFiles();

    void testMatrixSizeMismatch_data();
    void testMatrixSizeMismatch();

    void testConvertVersion_data();
    void testConvertVersion();
    void testConvertToNIFTI1Limits();
    void testSwappedMatrixChunks();

    void testPmodMetadata();
    void testDicomExtension();
    void testWriteMetadataExtensions_data();
    void testWriteMetadataExtensions();
    void testDicomTrailingZeros();
    void testTracerRadionuclide_data();
    void testTracerRadionuclide();
    void testConvertVersionAttributes();
    void testEcatFromNiftiHeader();
    void testSaveRewritesMetadata_data();
    void testSaveRewritesMetadata();
    void testWriteMatrixTiming_data();
    void testWriteMatrixTiming();
    void testOtherToolMetadata_data();
    void testOtherToolMetadata();

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
  QTest::newRow("Analyze 7.5 (hdr)")  << "analyze.hdr"           << false;
  QTest::newRow("Analyze 7.5 (img)")  << "analyze.img"           << false;
  QTest::newRow("NIfTI-1 pair (hdr)") << "n1_pair.hdr"           << true;
  QTest::newRow("NIfTI-1 pair (img)") << "n1_pair.img"           << true;
  QTest::newRow("NIfTI-1 pair ext")   << "p1_le_int16_ext.img"   << true;
  QTest::newRow("NIfTI-1 pair gzip (hdr)") << "p1_le_uint8.hdr.gz" << true;
  QTest::newRow("NIfTI-1 pair gzip (img)") << "p1_le_uint8.img.gz" << true;
  QTest::newRow("NIfTI-2 pair (hdr)") << "p2_be_float32_4d.hdr"  << true;
  QTest::newRow("NIfTI-2 pair (img)") << "p2_be_float32_4d.img"  << true;
  QTest::newRow("missing file")       << "missing.hdr"           << false;
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
  QTest::newRow("NIfTI-1 pair extensions")  << "p1_le_int16_ext.hdr"   << 1 << 1 << DT_INT16   << QSysInfo::LittleEndian << true;
  QTest::newRow("NIfTI-1 pair via img")     << "p1_le_int16_ext.img"   << 1 << 1 << DT_INT16   << QSysInfo::LittleEndian << true;
  QTest::newRow("NIfTI-1 pair gzip uint8")  << "p1_le_uint8.img.gz"    << 1 << 1 << DT_UINT8   << QSysInfo::LittleEndian << false;
  QTest::newRow("NIfTI-1 pair vox_offset")  << "p1_le_int16_offset.hdr" << 1 << 1 << DT_INT16  << QSysInfo::LittleEndian << false;
  QTest::newRow("NIfTI-2 pair BE float32 4D") << "p2_be_float32_4d.img" << 2 << 2 << DT_FLOAT32 << QSysInfo::BigEndian    << false;
}

// header/image pairs are identified by the file name
static bool pairName(const QString& name)
{
  QString n = name.toLower();
  if(n.endsWith(".gz"))
    n.chop(3);
  return n.endsWith(".hdr") || n.endsWith(".img");
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
  QCOMPARE(niftiFile.isPair(), pairName(file));
  if(niftiFile.isPair()) {
    QString base = dataFile(file);
    base.chop(base.endsWith(".gz") ? 7 : 4);
    const QString gz = file.endsWith(".gz") ? ".gz" : "";
    QCOMPARE(niftiFile.headerFileName(), base + ".hdr" + gz);
    QCOMPARE(niftiFile.imageFileName(), base + ".img" + gz);
  }

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
  QTest::addColumn<QString>("suffix");
  QTest::addColumn<QSysInfo::Endian>("byteOrder");
  QTest::addColumn<bool>("withJson");

  // single files and header/image pairs (pairs are written via the header
  // or the image file name)
  const char* suffixes[] = { ".nii", ".nii.gz", ".hdr", ".img.gz" };

  for(int version=1; version <= 2; version++) {
    for(const char* suffix : suffixes) {
      for(int bigEndian=0; bigEndian <= 1; bigEndian++) {
        for(int withJson=0; withJson <= 1; withJson++) {
          QString name = QString("NIfTI-%1 %2 %3%4").arg(version)
                                                     .arg(suffix)
                                                     .arg(bigEndian ? "BE" : "LE")
                                                     .arg(withJson ? " JSON" : "");
          QTest::newRow(name.toLatin1().constData()) << version << QString(suffix)
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
  QFETCH(QString, suffix);
  QFETCH(QSysInfo::Endian, byteOrder);
  QFETCH(bool, withJson);

  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  const QString fileName = tmpDir.filePath("test" + suffix);
  const bool pair = pairName(suffix);
  const QString gz = suffix.endsWith(".gz") ? ".gz" : "";
  const QString headerFile = pair ? tmpDir.filePath("test.hdr" + gz) : fileName;
  const QString imageFile = pair ? tmpDir.filePath("test.img" + gz) : fileName;

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
      if(withJson && pair == false)
        h->setVox_Offset(headerSize + 4 + h->headerExtensionSize(json));
    } else {
      CNIFTI2MainHeader* h = static_cast<CNIFTI2MainHeader*>(header.data());
      fillHeader(h, dims, byteOrder);
      if(withJson && pair == false)
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
  QVERIFY(QFile::exists(headerFile));
  QVERIFY(QFile::exists(imageFile));
  QByteArray content = fileContent(headerFile);
  QVERIFY(content.size() >= headerSize);

  QCOMPARE(fromByteOrder<quint32>(content.constData(), byteOrder), quint32(headerSize));

  qint64 voxOffset;
  if(version == 1) {
    QCOMPARE(QByteArray(content.constData()+344, 4), QByteArray(pair ? "ni1\0" : "n+1\0", 4));
    voxOffset = static_cast<qint64>(fromByteOrder<float>(content.constData()+108, byteOrder));
  } else {
    QCOMPARE(QByteArray(content.constData()+4, 8), QByteArray(pair ? "ni2\0\r\n\032\n" : "n+2\0\r\n\032\n", 8));
    voxOffset = fromByteOrder<qint64>(content.constData()+168, byteOrder);
  }

  QByteArray image = content;
  if(pair) {
    // the header file holds the header and its extensions only, the
    // image file the voxel data starting at vox_offset 0
    QCOMPARE(voxOffset, qint64(0));
    if(withJson) {
      QVERIFY(content.size() > headerSize + 4);
      QCOMPARE(content.at(headerSize) != 0, true); // extension indicator
      QCOMPARE((content.size() - headerSize - 4) % 16, 0);
    } else {
      QCOMPARE(content.size(), headerSize);
    }
    image = fileContent(imageFile);
    QCOMPARE(image.size(), matrix.size());
  } else {
    QVERIFY(content.size() > headerSize + 4);
    QVERIFY(voxOffset >= headerSize + 4);
    QCOMPARE(voxOffset % 16, qint64(0));
    QCOMPARE(qint64(content.size()), voxOffset + matrix.size());
    QCOMPARE(content.at(headerSize) != 0, withJson); // extension indicator
  }

  // first voxel in the requested byte order
  QCOMPARE(fromByteOrder<qint16>(image.constData()+voxOffset, byteOrder),
           static_cast<qint16>(patternValue(0, DT_INT16)));
  QCOMPARE(fromByteOrder<qint16>(image.constData()+voxOffset+2, byteOrder),
           static_cast<qint16>(patternValue(1, DT_INT16)));

  // read the file back (pairs via the other file name)
  CNIFTIFile niftiFile(pair ? (fileName == headerFile ? imageFile : headerFile) : fileName);
  QVERIFY(niftiFile.open(QIODevice::ReadOnly));
  QCOMPARE(niftiFile.isPair(), pair);

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
// special cases of header/image pairs
void TestNIFTIFormat::testPairFiles()
{
  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  const QString hdr = dataFile("n1_pair.hdr");
  const QString img = dataFile("n1_pair.img");

  // the image file is required
  QVERIFY(copyFile(hdr, tmpDir.filePath("noimage.hdr")));
  QVERIFY(CNIFTIFile::isOfType(tmpDir.filePath("noimage.hdr")) == false);
  {
    CNIFTIFile niftiFile(tmpDir.filePath("noimage.hdr"));
    QVERIFY(niftiFile.open(QIODevice::ReadOnly) == false);
  }

  // the magic has to match the file name: single file named .hdr/.img ...
  QVERIFY(copyFile(dataFile("n1_le_int16.nii"), tmpDir.filePath("single.hdr")));
  QVERIFY(copyFile(img, tmpDir.filePath("single.img")));
  QVERIFY(CNIFTIFile::isOfType(tmpDir.filePath("single.hdr")) == false);
  QVERIFY(CNIFTIFile::isOfType(tmpDir.filePath("single.img")) == false);
  {
    CNIFTIFile niftiFile(tmpDir.filePath("single.hdr"));
    QVERIFY(niftiFile.open(QIODevice::ReadOnly) == false);
  }

  // ... and the header of a pair named .nii
  QVERIFY(copyFile(hdr, tmpDir.filePath("pair.nii")));
  QVERIFY(CNIFTIFile::isOfType(tmpDir.filePath("pair.nii")) == false);

  // upper case and mixed case extensions
  QVERIFY(copyFile(hdr, tmpDir.filePath("UPPER.HDR")));
  QVERIFY(copyFile(img, tmpDir.filePath("UPPER.IMG")));
  QVERIFY(copyFile(hdr, tmpDir.filePath("mixed.hdr")));
  QVERIFY(copyFile(img, tmpDir.filePath("mixed.IMG")));
  foreach(const QString& name, QStringList() << "UPPER.IMG" << "UPPER.HDR" << "mixed.hdr" << "mixed.IMG") {
    QVERIFY2(CNIFTIFile::isOfType(tmpDir.filePath(name)), qPrintable(name));
    CNIFTIFile niftiFile(tmpDir.filePath(name));
    QVERIFY(niftiFile.open(QIODevice::ReadOnly));
    QByteArray* matrix = NULL;
    QVERIFY(niftiFile.readMatrix(matrix));
    QCOMPARE(matrix->size(), 4 * 3 * 2 * 2);
    QCOMPARE(voxelValue(matrix->constData(), 23, DT_INT16), patternValue(23, DT_INT16));
    delete matrix;
    niftiFile.close();
  }

  // trailing bytes in the image file are ignored, a truncated image file is rejected
  QVERIFY(copyFile(hdr, tmpDir.filePath("trailing.hdr")));
  QVERIFY(copyFile(img, tmpDir.filePath("trailing.img")));
  {
    QFile file(tmpDir.filePath("trailing.img"));
    QVERIFY(file.open(QIODevice::Append));
    file.write("trailing bytes");
    file.close();

    CNIFTIFile niftiFile(tmpDir.filePath("trailing.hdr"));
    QVERIFY(niftiFile.open(QIODevice::ReadOnly));
    QByteArray* matrix = NULL;
    QVERIFY(niftiFile.readMatrix(matrix));
    QCOMPARE(matrix->size(), 4 * 3 * 2 * 2);
    delete matrix;
  }

  QVERIFY(copyFile(hdr, tmpDir.filePath("truncated.hdr")));
  QVERIFY(copyFile(img, tmpDir.filePath("truncated.img")));
  {
    QFile file(tmpDir.filePath("truncated.img"));
    QVERIFY(file.resize(file.size() - 2));

    CNIFTIFile niftiFile(tmpDir.filePath("truncated.img"));
    QVERIFY(niftiFile.open(QIODevice::ReadOnly));
    QByteArray* matrix = NULL;
    QVERIFY(niftiFile.readMatrix(matrix) == false);
    QVERIFY(matrix == NULL);
  }

  // writing replaces both files of an existing pair
  QVERIFY(copyFile(hdr, tmpDir.filePath("replace.hdr")));
  QVERIFY(copyFile(dataFile("p2_be_float32_4d.img"), tmpDir.filePath("replace.img")));
  {
    CNIFTIFile niftiFile(tmpDir.filePath("replace.img"), CNIFTIMainHeader::NIFTI1);
    QVERIFY(niftiFile.open(QIODevice::WriteOnly));
    QScopedPointer<CNIFTIMainHeader> header(niftiFile.createEmptyHeader());
    const qint64 dims[8] = { 3, 2, 2, 2, 1, 1, 1, 1 };
    fillHeader(static_cast<CNIFTI1MainHeader*>(header.data()), dims, QSysInfo::LittleEndian);
    QVERIFY(niftiFile.writeMainHeader(*header));
    QVERIFY(niftiFile.writeMatrix(patternMatrix(8)));
    niftiFile.close();
  }
  QCOMPARE(fileContent(tmpDir.filePath("replace.hdr")).size(), 348);
  QCOMPARE(fileContent(tmpDir.filePath("replace.img")), patternMatrix(8));

  // no header file is left behind if the image file cannot be created
  QVERIFY(QDir(tmpDir.path()).mkdir("blocked.img"));
  {
    CNIFTIFile niftiFile(tmpDir.filePath("blocked.hdr"), CNIFTIMainHeader::NIFTI1);
    QVERIFY(niftiFile.open(QIODevice::WriteOnly) == false);
  }
  QVERIFY(QFile::exists(tmpDir.filePath("blocked.hdr")) == false);

  // an existing file is kept if no NIfTI version is given for writing
  QVERIFY(copyFile(dataFile("n1_le_int16.nii"), tmpDir.filePath("keep.nii")));
  {
    CNIFTIFile niftiFile(tmpDir.filePath("keep.nii"));
    QVERIFY(niftiFile.open(QIODevice::WriteOnly) == false);
  }
  QCOMPARE(fileContent(tmpDir.filePath("keep.nii")), fileContent(dataFile("n1_le_int16.nii")));

  // nor a compressed file if its directory does not exist
  {
    CNIFTIFile niftiFile(tmpDir.filePath("missing/dir.nii.gz"), CNIFTIMainHeader::NIFTI1);
    QVERIFY(niftiFile.open(QIODevice::WriteOnly) == false);
  }

  // a compressed file is written when it is closed: errorStatus() reports
  // whether that succeeded (here the file is replaced by a directory before)
  const qint64 dims[8] = { 3, 5, 4, 3, 1, 1, 1, 1 };
  const char* names[] = { "closed.nii.gz", "notclosed.nii.gz" };
  for(int n=0; n < 2; n++) {
    const QString fileName = tmpDir.filePath(names[n]);
    CNIFTIFile niftiFile(fileName, CNIFTIMainHeader::NIFTI1);
    QVERIFY(niftiFile.open(QIODevice::WriteOnly));

    QScopedPointer<CNIFTIMainHeader> header(niftiFile.createEmptyHeader());
    fillHeader(static_cast<CNIFTI1MainHeader*>(header.data()), dims, QSysInfo::LittleEndian);
    QVERIFY(niftiFile.writeMainHeader(*header));
    QVERIFY(niftiFile.writeMatrix(patternMatrix(5 * 4 * 3)));

    if(n == 1)
      QVERIFY(QFile::remove(fileName) && QDir(tmpDir.path()).mkdir(names[n]));

    niftiFile.close();
    QCOMPARE(niftiFile.errorStatus(), static_cast<int>(n == 0 ? CMedIOData::MERR_Ok : CMedIOData::MERR_Unspecified));
    QCOMPARE(QFileInfo(fileName).isFile(), n == 0);
  }
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

// a voxel matrix in the other byte order is swapped in chunks of 16 MB when
// it is written: 256 x 256 x 200 int16 voxels (26 MB) in big endian order
void TestNIFTIFormat::testSwappedMatrixChunks()
{
  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  const QString fileName = tmpDir.filePath("chunks.nii");
  const qint64 dims[8] = { 3, 256, 256, 200, 1, 1, 1, 1 };
  const qint64 voxels = 256 * 256 * 200;
  const QByteArray matrix = patternMatrix(voxels);
  QVERIFY(matrix.size() > 16 * 1024 * 1024);

  {
    CNIFTIFile niftiFile(fileName, CNIFTIMainHeader::NIFTI1);
    QVERIFY(niftiFile.open(QIODevice::WriteOnly));
    QScopedPointer<CNIFTIMainHeader> header(niftiFile.createEmptyHeader());
    fillHeader(static_cast<CNIFTI1MainHeader*>(header.data()), dims, QSysInfo::BigEndian);
    QVERIFY(niftiFile.writeMainHeader(*header));
    QVERIFY(niftiFile.writeMatrix(matrix.constData(), static_cast<unsigned int>(matrix.size())));
    niftiFile.close();
  }

  // big endian in the file
  const QByteArray content = fileContent(fileName);
  QCOMPARE(static_cast<qint64>(content.size()), 352 + voxels * 2);
  const char* data = content.constData() + 352;
  for(qint64 i=0; i < voxels; i++) {
    const qint16 value = qFromBigEndian<qint16>(reinterpret_cast<const uchar*>(data + i * 2));
    if(value != static_cast<qint16>(patternValue(i, DT_INT16)))
      QFAIL(qPrintable(QString("voxel %1: %2 instead of %3").arg(i).arg(value).arg(patternValue(i, DT_INT16))));
  }

  // and read back in machine byte order
  CNIFTIFile niftiFile(fileName);
  QVERIFY(niftiFile.open(QIODevice::ReadOnly));
  QByteArray* read = NULL;
  QVERIFY(niftiFile.readMatrix(read) && read != NULL);
  QVERIFY(*read == matrix);
  delete read;
  niftiFile.close();
}

//---------------------------------------------------------------------------------------------
// metadata in header extensions (see CNIFTIExtensions.h)

// header attributes of a NIfTI file read with the generic interface
static QJsonObject fileAttributes(const QString& fileName)
{
  QScopedPointer<CMedIOData> file(CMedIODataFactory::create(fileName));
  if(file.isNull() || file->open(QIODevice::ReadOnly) == false)
    return QJsonObject();
  CMedIOHeader* header = NULL;
  if(file->readMainHeader(header) == false || header == NULL)
    return QJsonObject();
  QScopedPointer<CMedIOHeader> guard(header);
  return header->attributes();
}

// codes and data of the header extensions of a NIfTI-1/NIfTI-2 single file or
// header file of a pair (raw bytes)
static QList<QPair<qint32, QByteArray> > rawExtensions(const QByteArray& content)
{
  QList<QPair<qint32, QByteArray> > list;
  qint32 sizeofHdr = 0;
  memcpy(&sizeofHdr, content.constData(), 4);
  const bool swap = (sizeofHdr != 348 && sizeofHdr != 540);
  if(swap)
    sizeofHdr = qbswap(sizeofHdr);
  const qint64 headerSize = sizeofHdr;

  // vox_offset: float at 108 (NIfTI-1), int64 at 168 (NIfTI-2); 0 for pairs
  qint64 voxOffset = 0;
  if(headerSize == 348) {
    float v = 0.0f;
    memcpy(&v, content.constData() + 108, 4);
    if(swap)
      v = qFromBigEndian<float>(content.constData() + 108);
    voxOffset = static_cast<qint64>(v);
  } else {
    memcpy(&voxOffset, content.constData() + 168, 8);
    if(swap)
      voxOffset = qbswap(voxOffset);
  }
  const qint64 end = (voxOffset > headerSize) ? voxOffset : content.size();
  if(content.size() < headerSize + 4 || content[static_cast<int>(headerSize)] == 0)
    return list;

  qint64 pos = headerSize + 4;
  while(pos + 8 <= end) {
    qint32 esize = 0, ecode = 0;
    memcpy(&esize, content.constData() + pos, 4);
    memcpy(&ecode, content.constData() + pos + 4, 4);
    if(swap) {
      esize = qbswap(esize);
      ecode = qbswap(ecode);
    }
    if(esize < 16 || esize % 16 != 0 || pos + esize > end)
      break;
    list.append(qMakePair(ecode, content.mid(static_cast<int>(pos + 8), esize - 8)));
    pos += esize;
  }
  return list;
}

// JSON object of the data of a JSON header extension (without padding)
static QJsonObject extensionJson(const QByteArray& data)
{
  QByteArray text = data;
  while(text.endsWith('\0'))
    text.chop(1);
  return QJsonDocument::fromJson(text).object();
}

// a file written by PMOD (data/pmod_le_int16_4d.nii, see generate_fixtures.py):
// the units and frame timing of its DICOM extension and the patient name, study
// type and description of descrip
void TestNIFTIFormat::testPmodMetadata()
{
  const QString fileName = dataFile("pmod_le_int16_4d.nii");
  const QJsonObject a = fileAttributes(fileName);
  QCOMPARE(a.value("PatientName").toString(), QString("Test^Patient"));
  QCOMPARE(a.value("StudyDescription").toString(), QString("PET test study"));
  QCOMPARE(a.value("ECAT7").toObject().value("Study_Type").toString(), QString("DYNAMIC\\IMA"));
  QCOMPARE(a.value("Units").toString(), QString("BQML"));
  QCOMPARE(a.value("FrameTimesStart").toArray(), (QJsonArray{0, 15, 45}));
  QCOMPARE(a.value("FrameDuration").toArray(), (QJsonArray{15, 30, 45}));

  // the timing of the volumes
  QScopedPointer<CMedIOData> file(CMedIODataFactory::create(fileName));
  QVERIFY(file->open(QIODevice::ReadOnly));
  CMedIOVolume v;
  QVERIFY(file->readVolume(v, 3));
  QCOMPARE(v.frameStart(), 45.0);
  QCOMPARE(v.frameDuration(), 45.0);
  file->close();

  // converted into ECAT7
  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  const QString ecatName = tmpDir.filePath("pmod.v");
  QString error;
  QVERIFY2(CNIFTIConverter::niftiToEcat(fileName, ecatName, &error), qPrintable(error));
  CECATFile ecat(ecatName);
  QVERIFY(ecat.open(QIODevice::ReadOnly));
  CECATMainHeader* mh = NULL;
  QVERIFY(ecat.readMainHeader(mh) && mh != NULL);
  QScopedPointer<CECATMainHeader> mhGuard(mh);
  const CECAT7MainHeader* m = static_cast<const CECAT7MainHeader*>(mh);
  QCOMPARE(QString(m->patient_Name()), QString("Test^Patient"));
  QCOMPARE(QString(m->study_Type()), QString("DYNAMIC\\IMA"));
  QCOMPARE(QString(m->study_Description()), QString("PET test study"));
  QCOMPARE(QString(m->data_Units()), QString("Bq/ml"));
  const unsigned int starts[3] = { 0, 15000, 45000 }, durations[3] = { 15000, 30000, 45000 };
  for(short f=1; f <= 3; f++) {
    CECATSubHeader* sh = NULL;
    QVERIFY(ecat.readSubHeader(sh, f) && sh != NULL);
    QScopedPointer<CECATSubHeader> shGuard(sh);
    QCOMPARE(static_cast<const CECAT7SubHeaderImage*>(sh)->frame_Start_Time(), starts[f - 1]);
    QCOMPARE(static_cast<const CECAT7SubHeaderImage*>(sh)->frame_Duration(), durations[f - 1]);
  }
}

// a DICOM extension with attributes in sequences and a Latin-1 patient name
// (big endian, data/n1_be_float32_dicom.nii): the values the JSON extension
// does not have are taken from it
void TestNIFTIFormat::testDicomExtension()
{
  const QJsonObject a = fileAttributes(dataFile("n1_be_float32_dicom.nii"));

  // JSON extension first
  QCOMPARE(a.value("PatientWeight").toDouble(), 70.5);
  QCOMPARE(a.value("StudyDescription").toString(), QString("from JSON"));

  // DICOM extension
  QCOMPARE(a.value("PatientName").toString(), QString::fromUtf8("M\xc3\xbcller^Hans"));
  QCOMPARE(a.value("PatientID").toString(), QString("ID-0815"));
  QCOMPARE(a.value("PatientBirthDate").toString(), QString("1970-01-02"));
  QCOMPARE(a.value("PatientSex").toString(), QString("M"));
  QCOMPARE(a.value("PatientSize").toDouble(), 1.78);
  QCOMPARE(a.value("AcquisitionDateTime").toString(), QString("2024-02-27T14:13:44"));
  QCOMPARE(a.value("Radiopharmaceutical").toString(), QString("FDG"));
  QCOMPARE(a.value("RadiopharmaceuticalStartDateTime").toString(), QString("2024-02-27T13:15:00"));
  QCOMPARE(a.value("RadionuclideTotalDose").toDouble(), 370000000.0);
  QCOMPARE(a.value("RadionuclideHalfLife").toDouble(), 6586.2);
  const QJsonObject code = a.value("RadionuclideCodeSequence").toObject();
  QCOMPARE(code.value("CodeValue").toString(), QString("C-111A1"));
  QCOMPARE(code.value("CodingSchemeDesignator").toString(), QString("SRT"));
  QCOMPARE(code.value("CodeMeaning").toString(), QString("^18^Fluorine"));
  QCOMPARE(a.value("Units").toString(), QString("BQML"));
  QCOMPARE(a.value("DecayCorrection").toString(), QString("START"));
  QCOMPARE(a.value("FrameTimesStart").toArray(), (QJsonArray{0, 60}));
  QCOMPARE(a.value("FrameDuration").toArray(), (QJsonArray{60, 120}));
}

void TestNIFTIFormat::testWriteMetadataExtensions_data()
{
  QTest::addColumn<QSysInfo::Endian>("byteOrder");

  QTest::newRow("little endian") << QSysInfo::LittleEndian;
  QTest::newRow("big endian")    << QSysInfo::BigEndian;
}

// the attributes of a header are written as JSON extension (first, as before)
// and as DICOM extension (explicit VR little endian, also in big endian files),
// descrip like PMOD; the DICOM extension alone gives the attributes as well
// (the data set is checked independently with pydicom by the Python tests)
void TestNIFTIFormat::testWriteMetadataExtensions()
{
  QFETCH(QSysInfo::Endian, byteOrder);

  // BIDS TimeZero is the local time of day (Central European Time, UTC+1)
  MedIOTestTimeZone timeZone("Europe/Berlin");

  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  const QString fileName = tmpDir.filePath("metadata.nii");
  const qint64 dims[8] = { 4, 4, 3, 2, 2, 1, 1, 1 };

  QJsonObject attributes;
  attributes["PatientName"] = QString::fromUtf8("M\xc3\xbcller^Hans");
  attributes["PatientWeight"] = 72.25;
  attributes["StudyDescription"] = QString("brain study");
  attributes["AcquisitionDateTime"] = QString("2024-02-27T14:13:44Z");
  attributes["RadiopharmaceuticalStartDateTime"] = QString("2024-02-27T13:43:44Z");
  attributes["RadionuclideTotalDose"] = 3.7e8;
  attributes["RadionuclideCodeSequence"] = QJsonObject{{"CodeValue", "C-111A1"}, {"CodingSchemeDesignator", "SRT"},
                                                       {"CodeMeaning", "^18^Fluorine"}};
  attributes["Units"] = QString("BQML");
  attributes["DecayCorrection"] = QString("ADMIN");
  attributes["ECAT7"] = QJsonObject{{"Study_Type", "STATIC"}};
  attributes["FrameTimesStart"] = QJsonArray{0, 60.5};
  attributes["FrameDuration"] = QJsonArray{60.5, 120};

  {
    CNIFTIFile niftiFile(fileName, CNIFTIMainHeader::NIFTI1);
    QVERIFY(niftiFile.open(QIODevice::WriteOnly));
    QScopedPointer<CNIFTIMainHeader> header(niftiFile.createEmptyHeader());
    fillHeader(static_cast<CNIFTI1MainHeader*>(header.data()), dims, byteOrder);
    static_cast<CNIFTI1MainHeader*>(header.data())->setDescrip("converted by libmedio");
    QVERIFY(header->updateAttributes(attributes));
    QVERIFY(niftiFile.writeMainHeader(*header));
    QVERIFY(niftiFile.writeMatrix(patternMatrix(4 * 3 * 2 * 2)));
    niftiFile.close();
  }

  // descrip like PMOD writes it, JSON extension first, then the DICOM extension
  QByteArray content = fileContent(fileName);
  QCOMPARE(QString::fromLatin1(content.constData() + 148, qstrnlen(content.constData() + 148, 80)),
           QString::fromLatin1("M\xfcller^Hans [STATIC] brain study"));
  const QList<QPair<qint32, QByteArray> > extensions = rawExtensions(content);
  QCOMPARE(extensions.size(), 2);
  QCOMPARE(extensions[0].first, 6);
  QCOMPARE(extensions[1].first, 2);

  // BIDS keys of the JSON extension
  QByteArray text = extensions[0].second;
  while(text.endsWith('\0'))
    text.chop(1);
  const QJsonObject json = QJsonDocument::fromJson(text).object();
  QCOMPARE(json.value("Units").toString(), QString("Bq/mL"));
  QCOMPARE(json.value("TracerRadionuclide").toString(), QString("F18"));
  QCOMPARE(json.value("TimeZero").toString(), QString("15:13:44"));  // 14:13:44 UTC
  QCOMPARE(json.value("ScanStart").toDouble(), 0.0);
  QCOMPARE(json.value("InjectionStart").toDouble(), -1800.0);
  QCOMPARE(json.value("ImageDecayCorrected").toBool(), true);
  QCOMPARE(json.value("ImageDecayCorrectionTime").toDouble(), -1800.0);
  QCOMPARE(json.value("InjectedRadioactivity").toDouble(), 370.0);

  // DICOM data set: explicit VR little endian, starting with (0008,0005) CS
  const QByteArray dicom = extensions[1].second;
  QCOMPARE(dicom.left(6), QByteArray("\x08\x00\x05\x00" "CS", 6));

  // without the JSON extension (code changed to 0, ignored): the attributes of the DICOM extension
  const int jsonCodeOffset = 352 + 4;
  const qint32 ignore = 0;
  memcpy(content.data() + jsonCodeOffset, &ignore, 4);
  const QString dicomOnly = tmpDir.filePath("dicom-only.nii");
  {
    QFile f(dicomOnly);
    QVERIFY(f.open(QIODevice::WriteOnly));
    QCOMPARE(f.write(content), static_cast<qint64>(content.size()));
  }
  const QJsonObject a = fileAttributes(dicomOnly);
  foreach(const QString& key, QStringList() << "PatientName" << "PatientWeight" << "StudyDescription"
                                            << "AcquisitionDateTime" << "RadiopharmaceuticalStartDateTime"
                                            << "RadionuclideTotalDose" << "RadionuclideCodeSequence"
                                            << "Units" << "DecayCorrection" << "FrameTimesStart" << "FrameDuration")
    QVERIFY2(a.value(key) == attributes.value(key), qPrintable(key));
}

// readers like nibabel remove the trailing zero bytes of an extension: a data
// set whose last value ends with a zero byte (here a frame duration of 0) is
// completed by trailing padding, so that it can still be read without them
void TestNIFTIFormat::testDicomTrailingZeros()
{
  QJsonObject json;
  json["Units"] = QString("Bq/mL");
  json["FrameTimesStart"] = QJsonArray{0, 30};
  json["FrameDuration"] = QJsonArray{30, 0};

  QByteArray dicom = NIFTIExtensions::dicomDataSet(json);
  QVERIFY(dicom.endsWith('\0') == false);
  while(dicom.endsWith('\0'))
    dicom.chop(1);
  bool pmod = false;
  const QJsonObject read = NIFTIExtensions::metadataOfDicomDataSet(dicom, &pmod);
  QVERIFY(pmod);
  QCOMPARE(read.value("Units").toString(), QString("BQML"));
  QCOMPARE(read.value("FrameDuration").toArray(), (QJsonArray{30, 0}));
}

// BIDS TracerRadionuclide (chemical symbol and mass number, e.g. "F18") of the
// isotope names of the formats and the radionuclide code of the attributes
void TestNIFTIFormat::testTracerRadionuclide_data()
{
  QTest::addColumn<QString>("section");
  QTest::addColumn<QString>("name");
  QTest::addColumn<QString>("expected");

  QTest::newRow("ecat-symbol") << "ECAT7" << "F-18" << "F18";
  QTest::newRow("ecat-metastable") << "ECAT7" << "Tc-99m" << "Tc99m";
  QTest::newRow("ecat-mass-first") << "ECAT7" << "64Cu" << "Cu64";
  QTest::newRow("ecat-lanthanum") << "ECAT7" << "La-133" << "La133";
  QTest::newRow("ecat-lanthanum-name") << "ECAT7" << "Lanthanum-133" << "La133";
  QTest::newRow("philips-gallium") << "Philips" << "Ga-68" << "Ga68";
  QTest::newRow("code-fluorine") << "RadionuclideCodeSequence" << "^18^Fluorine" << "F18";
  QTest::newRow("code-lanthanum") << "RadionuclideCodeSequence" << "^133^Lanthanum" << "La133";
  QTest::newRow("code-unknown") << "RadionuclideCodeSequence" << "^1^Unobtainium" << "";
}

void TestNIFTIFormat::testTracerRadionuclide()
{
  QFETCH(QString, section);
  QFETCH(QString, name);
  QFETCH(QString, expected);

  const QString key = section == "ECAT7" ? "Isotope_Name"
                    : section == "Philips" ? "isotope_name" : "CodeMeaning";
  QJsonObject attributes;
  attributes["PatientName"] = QString("Doe^Jane");
  attributes[section] = QJsonObject{{key, name}};

  const QJsonObject json = MedIOHeaderMapping::niftiJson(attributes);
  QCOMPARE(json.contains("TracerRadionuclide"), expected.isEmpty() == false);
  QCOMPARE(json.value("TracerRadionuclide").toString(), expected);
}

//---------------------------------------------------------------------------------------------
// applications using the NIfTI classes directly (not CMedIOData::writeVolume())
// get the same metadata as the files of CNIFTIConverter

// a header converted into the other NIfTI version keeps the attributes (the
// header extensions of data/n1_be_float32_dicom.nii, see generate_fixtures.py)
void TestNIFTIFormat::testConvertVersionAttributes()
{
  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  const QString names[3] = { QString(NIFTI_TEST_DATA_DIR) + "/n1_be_float32_dicom.nii",
                             tmpDir.filePath("nifti2.nii"), tmpDir.filePath("nifti1.hdr") };
  const CNIFTIMainHeader::Type types[2] = { CNIFTIMainHeader::NIFTI2, CNIFTIMainHeader::NIFTI1 };

  // NIfTI-1 (fixture) -> NIfTI-2 -> NIfTI-1 header/image pair
  for(int i=0; i < 2; i++) {
    CNIFTIFile source(names[i]);
    QVERIFY(source.open(QIODevice::ReadOnly));
    CNIFTIMainHeader* h = NULL;
    QVERIFY(source.readMainHeader(h) && h != NULL);
    QScopedPointer<CNIFTIMainHeader> guard(h);
    QByteArray* data = NULL;
    QVERIFY(source.readMatrix(data) && data != NULL);
    QScopedPointer<QByteArray> dataGuard(data);

    CNIFTIFile target(names[i + 1], types[i]);
    QVERIFY(target.open(QIODevice::WriteOnly));
    QScopedPointer<CNIFTIMainHeader> header(target.createEmptyHeader());
    QVERIFY(header->convertFrom(h));
    QVERIFY(target.writeMainHeader(*header));
    QVERIFY(target.writeMatrix(*data));
    target.close();

    // values of the JSON extension, then of the DICOM extension (fixture)
    const QJsonObject a = fileAttributes(names[i + 1]);
    QCOMPARE(a.value("PatientWeight").toDouble(), 70.5);
    QCOMPARE(a.value("StudyDescription").toString(), QString("from JSON"));
    QCOMPARE(a.value("PatientName").toString(), QString::fromUtf8("M\xc3\xbcller^Hans"));
    QCOMPARE(a.value("PatientID").toString(), QString("ID-0815"));
    QCOMPARE(a.value("Units").toString(), QString("BQML"));
    QCOMPARE(a.value("FrameTimesStart").toArray(), (QJsonArray{0, 60}));
    QCOMPARE(a.value("FrameDuration").toArray(), (QJsonArray{60, 120}));

    const QList<QPair<qint32, QByteArray> > extensions = rawExtensions(fileContent(names[i + 1]));
    QCOMPARE(extensions.size(), 2);
    QCOMPARE(extensions[0].first, 6);
    QCOMPARE(extensions[1].first, 2);
    QCOMPARE(extensionJson(extensions[0].second).value("TracerRadionuclide").toString(), QString("F18"));
  }
}

// an ECAT7 main and sub header converted from a NIfTI header take the
// attributes and the frame timing of its header extensions (fixture values)
void TestNIFTIFormat::testEcatFromNiftiHeader()
{
  CNIFTIFile source(QString(NIFTI_TEST_DATA_DIR) + "/n1_be_float32_dicom.nii");
  QVERIFY(source.open(QIODevice::ReadOnly));
  CNIFTIMainHeader* h = NULL;
  QVERIFY(source.readMainHeader(h) && h != NULL);
  QScopedPointer<CNIFTIMainHeader> guard(h);

  CECAT7MainHeader mainHeader;
  QVERIFY(mainHeader.convertFrom(h));
  QCOMPARE(QString::fromLatin1(mainHeader.patient_Name()), QString::fromUtf8("M\xc3\xbcller^Hans"));
  QCOMPARE(mainHeader.patient_Weight(), 70.5f);
  QCOMPARE(QString::fromLatin1(mainHeader.study_Description()), QString("from JSON"));
  QCOMPARE(QString::fromLatin1(mainHeader.radiopharmaceutical()), QString("FDG"));
  QCOMPARE(mainHeader.num_Frames(), short(2));
  const QJsonObject a = mainHeader.attributes();
  QCOMPARE(a.value("PatientID").toString(), QString("ID-0815"));
  QCOMPARE(a.value("Units").toString(), QString("BQML"));
  QCOMPARE(a.value("RadionuclideTotalDose").toDouble(), 3.7e8);

  // the first frame: timing of the metadata (header: toffset 0, pixdim[4] 1)
  CECAT7SubHeaderImage subHeader;
  QVERIFY(subHeader.convertFrom(NULL, h));
  QCOMPARE(subHeader.frame_Start_Time(), 0u);
  QCOMPARE(subHeader.frame_Duration(), 60000u);
}

// save() of a header read from a file opened for reading and writing (e.g.
// to anonymize it) rewrites the metadata extensions and descrip completely:
// changed and removed values do not remain in the file, the voxel data is kept
void TestNIFTIFormat::testSaveRewritesMetadata_data()
{
  QTest::addColumn<QString>("name");
  QTest::addColumn<int>("version");

  QTest::newRow("nifti1 single") << "anon.nii" << 1;
  QTest::newRow("nifti1 pair") << "anon.hdr" << 1;
  QTest::newRow("nifti2 single") << "anon.nii" << 2;
}

void TestNIFTIFormat::testSaveRewritesMetadata()
{
  QFETCH(QString, name);
  QFETCH(int, version);

  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  const QString fileName = tmpDir.filePath(name);
  const CNIFTIMainHeader::Type type = (version == 2) ? CNIFTIMainHeader::NIFTI2 : CNIFTIMainHeader::NIFTI1;
  const qint64 dims[8] = { 3, 4, 3, 2, 1, 1, 1, 1 };
  const QByteArray voxels = patternMatrix(4 * 3 * 2);

  {
    CNIFTIFile file(fileName, type);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QScopedPointer<CNIFTIMainHeader> header(file.createEmptyHeader());
    // descrip empty: written for the attributes (an own descrip is kept)
    if(version == 2) {
      fillHeader(static_cast<CNIFTI2MainHeader*>(header.data()), dims, QSysInfo::LittleEndian);
      static_cast<CNIFTI2MainHeader*>(header.data())->setDescrip("");
    } else {
      fillHeader(static_cast<CNIFTI1MainHeader*>(header.data()), dims, QSysInfo::LittleEndian);
      static_cast<CNIFTI1MainHeader*>(header.data())->setDescrip("");
    }
    QVERIFY(header->updateAttributes(QJsonObject{{"PatientName", "Doe^Jane"}, {"PatientID", "ID-4711"},
                                                 {"PatientWeight", 81.5}, {"StudyDescription", "secret study"}}));
    QVERIFY(file.writeMainHeader(*header));
    QVERIFY(file.writeMatrix(voxels));
    file.close();
  }
  QVERIFY(fileContent(fileName).contains("Doe^Jane [] secret study"));  // descrip
  QVERIFY(fileContent(fileName).contains("ID-4711"));
  QCOMPARE(fileAttributes(fileName).value("PatientWeight").toDouble(), 81.5);

  const char* removed[] = { "Doe^Jane", "ID-4711", "secret study", "81.5" };
  const qint64 headerSize = (version == 2) ? 540 : 348;
  const int descripOffset = (version == 2) ? 240 : 148;

  for(int step=0; step < 2; step++) {
    {
      CNIFTIFile file(fileName);
      QVERIFY(file.open(QIODevice::ReadWrite));
      CNIFTIMainHeader* h = NULL;
      QVERIFY(file.readMainHeader(h) && h != NULL);
      QScopedPointer<CNIFTIMainHeader> guard(h);

      QJsonObject changes;
      if(step == 0) {
        // anonymized: another name, the other values removed
        changes["PatientName"] = QString("Anonymous");
        changes["PatientID"] = QJsonValue(QJsonValue::Null);
        changes["PatientWeight"] = QJsonValue(QJsonValue::Null);
        changes["StudyDescription"] = QJsonValue(QJsonValue::Null);
      } else {
        // all attributes removed
        foreach(const QString& key, h->attributes().keys())
          if(key != "NIfTI")
            changes[key] = QJsonValue(QJsonValue::Null);
      }
      QVERIFY(h->updateAttributes(changes));
      QVERIFY(h->save());
      file.close();
    }

    const QByteArray content = fileContent(fileName);
    for(unsigned i=0; i < sizeof(removed) / sizeof(removed[0]); i++)
      QVERIFY2(content.contains(removed[i]) == false, removed[i]);
    const QString descrip = QString::fromLatin1(content.constData() + descripOffset,
                                                static_cast<int>(qstrnlen(content.constData() + descripOffset, 80)));
    const QJsonObject a = fileAttributes(fileName);
    if(step == 0) {
      QCOMPARE(descrip, QString("Anonymous []"));
      QCOMPARE(a.value("PatientName").toString(), QString("Anonymous"));
      QVERIFY(a.contains("PatientID") == false);
      QVERIFY(a.contains("PatientWeight") == false);
      const QList<QPair<qint32, QByteArray> > extensions = rawExtensions(content);
      QCOMPARE(extensions.size(), 2);
      QCOMPARE(extensionJson(extensions[0].second).value("PatientName").toString(), QString("Anonymous"));
    } else {
      QVERIFY(content.contains("Anonymous") == false);
      QCOMPARE(descrip, QString());
      QCOMPARE(int(content[static_cast<int>(headerSize)]), 0);  // no extensions
      QVERIFY(a.contains("PatientName") == false);
    }

    // the voxel data is kept
    CNIFTIFile file(fileName);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray* data = NULL;
    QVERIFY(file.readMatrix(data) && data != NULL);
    QScopedPointer<QByteArray> dataGuard(data);
    QVERIFY(*data == voxels);
  }
}

// writeMatrix(data, frame, start, duration) stores the timing of every volume
// in the metadata extensions (JSON and the frame timing of PMOD), also
// without attributes and in any order of the volumes
void TestNIFTIFormat::testWriteMatrixTiming_data()
{
  QTest::addColumn<QString>("name");
  QTest::addColumn<int>("version");

  QTest::newRow("nifti1 single") << "timing.nii" << 1;
  QTest::newRow("nifti1 compressed") << "timing.nii.gz" << 1;
  QTest::newRow("nifti1 pair") << "timing.hdr" << 1;
  QTest::newRow("nifti2 single") << "timing.nii" << 2;
}

void TestNIFTIFormat::testWriteMatrixTiming()
{
  QFETCH(QString, name);
  QFETCH(int, version);

  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  const QString fileName = tmpDir.filePath(name);
  const qint64 dims[8] = { 4, 4, 3, 2, 3, 1, 1, 1 };
  const int volumeSize = 4 * 3 * 2 * 2;
  const QByteArray voxels = patternMatrix(4 * 3 * 2 * 3);
  const double starts[3] = { 0.0, 30.5, 90.0 };
  const double durations[3] = { 30.5, 59.5, 90.0 };

  {
    CNIFTIFile file(fileName, version == 2 ? CNIFTIMainHeader::NIFTI2 : CNIFTIMainHeader::NIFTI1);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QScopedPointer<CNIFTIMainHeader> header(file.createEmptyHeader());
    if(version == 2)
      fillHeader(static_cast<CNIFTI2MainHeader*>(header.data()), dims, QSysInfo::LittleEndian);
    else
      fillHeader(static_cast<CNIFTI1MainHeader*>(header.data()), dims, QSysInfo::LittleEndian);
    QVERIFY(file.writeMainHeader(*header));

    // invalid timing, size or volume: nothing is written
    QVERIFY(file.writeMatrix(voxels.mid(0, volumeSize), 1, -1.0, 10.0) == false);
    QVERIFY(file.writeMatrix(voxels.mid(0, volumeSize - 2), 1, 0.0, 10.0) == false);
    QVERIFY(file.writeMatrix(voxels.mid(0, volumeSize), 4, 0.0, 10.0) == false);

    const int order[3] = { 2, 0, 1 };
    for(int i=0; i < 3; i++) {
      const int k = order[i];
      QVERIFY(file.writeMatrix(voxels.mid(k * volumeSize, volumeSize), static_cast<short>(k + 1), starts[k], durations[k]));
    }
    file.close();
  }

  const QJsonObject a = fileAttributes(fileName);
  QCOMPARE(a.value("FrameTimesStart").toArray(), (QJsonArray{0, 30.5, 90}));
  QCOMPARE(a.value("FrameDuration").toArray(), (QJsonArray{30.5, 59.5, 90}));

  const QList<QPair<qint32, QByteArray> > extensions = rawExtensions(fileContent(fileName));
  QCOMPARE(extensions.size(), 2);
  QCOMPARE(extensions[0].first, 6);
  QCOMPARE(extensions[1].first, 2);
  QCOMPARE(extensionJson(extensions[0].second).value("FrameDuration").toArray(), (QJsonArray{30.5, 59.5, 90}));
  bool pmod = false;
  const QJsonObject dicom = NIFTIExtensions::metadataOfDicomDataSet(extensions[1].second, &pmod);
  QVERIFY(pmod);
  const QJsonArray pmodDurations = dicom.value("FrameDuration").toArray();
  QCOMPARE(pmodDurations.size(), 3);
  for(int k=0; k < 3; k++) {
    QVERIFY(std::fabs(dicom.value("FrameTimesStart").toArray()[k].toDouble() - starts[k]) < 1e-9);
    QVERIFY(std::fabs(pmodDurations[k].toDouble() - durations[k]) < 1e-9);
  }

  // the voxel data of all volumes
  CNIFTIFile file(fileName);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QByteArray* data = NULL;
  QVERIFY(file.readMatrix(data) && data != NULL);
  QScopedPointer<QByteArray> dataGuard(data);
  QVERIFY(*data == voxels);
}

//---------------------------------------------------------------------------------------------
// metadata of other tools: a JSON sidecar as dcm2niix (before 2023) writes it
// for DICOM PET series (DICOM keywords, local time without time zone,
// FrameDuration and FrameReferenceTime but no FrameTimesStart, the half-life
// but no radionuclide) and BIDS PET keys (TimeZero, InjectionStart,
// TracerRadionuclide). Expected values: computed by hand from the sidecar
// (frame start = reference time - duration / 2, local time of the time zone).
void TestNIFTIFormat::testOtherToolMetadata_data()
{
  QTest::addColumn<QByteArray>("zone");
  QTest::addColumn<QJsonObject>("sidecar");
  QTest::addColumn<QString>("acquisition");
  QTest::addColumn<QString>("injection");
  QTest::addColumn<QJsonArray>("starts");
  QTest::addColumn<QString>("isotope");
  QTest::addColumn<QString>("meaning");

  const QJsonObject dcm2niix{{"Modality", "PT"}, {"PatientName", "Phantom"}, {"PatientWeight", 8.63},
                             {"AcquisitionDateTime", "2026-06-03T19:04:56.837000"}, {"Radiopharmaceutical", "FDG"},
                             {"RadionuclideTotalDose", 6.078e7}, {"RadionuclideHalfLife", 6586.2}, {"Units", "BQML"},
                             {"SeriesDescription", "series"}, {"FrameDuration", QJsonArray{30, 60, 90}},
                             {"FrameReferenceTime", QJsonArray{15, 60, 135}}};

  // the scan in Central European Summer Time (UTC+2) and Japan Standard Time (UTC+9)
  QTest::newRow("dcm2niix Europe/Berlin") << QByteArray("Europe/Berlin") << dcm2niix << "2026-06-03T17:04:56Z" << ""
                                          << QJsonArray{0, 30, 90} << "F-18" << "^18^Fluorine";
  QTest::newRow("dcm2niix Asia/Tokyo") << QByteArray("Asia/Tokyo") << dcm2niix << "2026-06-03T10:04:56Z" << ""
                                       << QJsonArray{0, 30, 90} << "F-18" << "^18^Fluorine";

  // reference times of frames with gaps, overlapping (not the middle of the
  // frames: the frames follow each other) or none
  QJsonObject gaps = dcm2niix;
  gaps["FrameReferenceTime"] = QJsonArray{15, 75, 165};
  QTest::newRow("reference times with gaps") << QByteArray("UTC") << gaps << "2026-06-03T19:04:56Z" << ""
                                             << QJsonArray{0, 45, 120} << "F-18" << "^18^Fluorine";
  QJsonObject overlapping = dcm2niix;
  overlapping["FrameReferenceTime"] = QJsonArray{15, 40, 135};
  QTest::newRow("overlapping reference times") << QByteArray("UTC") << overlapping << "2026-06-03T19:04:56Z" << ""
                                               << QJsonArray{0, 30, 90} << "F-18" << "^18^Fluorine";
  QJsonObject durations = dcm2niix;
  durations.remove("FrameReferenceTime");
  durations["RadionuclideHalfLife"] = 1.0e9;   // no known radionuclide
  QTest::newRow("durations only") << QByteArray("UTC") << durations << "2026-06-03T19:04:56Z" << ""
                                  << QJsonArray{0, 30, 90} << "" << "";

  // BIDS PET: injection 5096 s before TimeZero 19:04:56 CEST = 15:40:00 UTC
  const QJsonObject bids{{"PatientName", "Phantom"}, {"AcquisitionDateTime", "2026-06-03T19:04:56"},
                         {"TimeZero", "19:04:56"}, {"ScanStart", 0}, {"InjectionStart", -5096},
                         {"TracerRadionuclide", "Cu64"}, {"SeriesDescription", "series"},
                         {"FrameTimesStart", QJsonArray{0, 30, 90}}, {"FrameDuration", QJsonArray{30, 60, 90}}};
  QTest::newRow("BIDS PET") << QByteArray("Europe/Berlin") << bids << "2026-06-03T17:04:56Z" << "2026-06-03T15:40:00Z"
                            << QJsonArray{0, 30, 90} << "Cu-64" << "^64^Copper";
}

void TestNIFTIFormat::testOtherToolMetadata()
{
  QFETCH(QByteArray, zone);
  QFETCH(QJsonObject, sidecar);
  QFETCH(QString, acquisition);
  QFETCH(QString, injection);
  QFETCH(QJsonArray, starts);
  QFETCH(QString, isotope);
  QFETCH(QString, meaning);

  MedIOTestTimeZone timeZone(zone.constData());
  QTemporaryDir tmpDir;
  QVERIFY(tmpDir.isValid());
  const QString fileName = tmpDir.filePath("tool.nii");
  const qint64 dims[8] = { 4, 4, 3, 2, 3, 1, 1, 1 };
  {
    CNIFTIFile file(fileName, CNIFTIMainHeader::NIFTI1);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QScopedPointer<CNIFTIMainHeader> header(file.createEmptyHeader());
    fillHeader(static_cast<CNIFTI1MainHeader*>(header.data()), dims, QSysInfo::LittleEndian);
    QVERIFY(file.writeMainHeader(*header));
    QVERIFY(file.writeMatrix(patternMatrix(4 * 3 * 2 * 3)));
    file.close();
  }
  QFile json(tmpDir.filePath("tool.json"));
  QVERIFY(json.open(QIODevice::WriteOnly));
  json.write(QJsonDocument(sidecar).toJson());
  json.close();

  // the attributes of the file
  const QJsonObject a = fileAttributes(fileName);
  QCOMPARE(a.value("FrameTimesStart").toArray(), starts);
  QCOMPARE(a.value("FrameDuration").toArray(), (QJsonArray{30, 60, 90}));
  QCOMPARE(a.value("ECAT7").toObject().value("Isotope_Name").toString(), isotope);
  QCOMPARE(a.value("RadionuclideCodeSequence").toObject().value("CodeMeaning").toString(), meaning);
  QCOMPARE(a.value("RadiopharmaceuticalStartDateTime").toString(), injection);

  // the volumes read with the generic interface
  QScopedPointer<CMedIOData> data(CMedIODataFactory::create(fileName));
  QVERIFY(data.isNull() == false && data->open(QIODevice::ReadOnly));
  for(int k=0; k < 3; k++) {
    CMedIOVolume v;
    QVERIFY(data->readVolume(v, static_cast<short>(k + 1)));
    QCOMPARE(v.frameStart(), starts[k].toDouble());
    QCOMPARE(v.frameDuration(), 30.0 * (k + 1));
  }

  // the ECAT7 headers of the NIfTI header
  CNIFTIFile file(fileName);
  QVERIFY(file.open(QIODevice::ReadOnly));
  CNIFTIMainHeader* h = NULL;
  QVERIFY(file.readMainHeader(h) && h != NULL);
  QScopedPointer<CNIFTIMainHeader> guard(h);
  CECAT7MainHeader mainHeader;
  QVERIFY(mainHeader.convertFrom(h));
  const QJsonObject e = mainHeader.attributes();
  QCOMPARE(e.value("AcquisitionDateTime").toString(), acquisition);
  QCOMPARE(e.value("RadiopharmaceuticalStartDateTime").toString(), injection);
  QCOMPARE(QString::fromLatin1(mainHeader.isotope_Name()), isotope.isEmpty() ? QString("N/A") : isotope);
  QCOMPARE(QString::fromLatin1(mainHeader.study_Description()), QString("series"));
  CECAT7SubHeaderImage subHeader;
  QVERIFY(subHeader.convertFrom(NULL, h));
  QCOMPARE(subHeader.frame_Start_Time(), 0u);
  QCOMPARE(subHeader.frame_Duration(), 30000u);
}

QTEST_GUILESS_MAIN(TestNIFTIFormat)
#include "testNIFTIFormat.moc"
