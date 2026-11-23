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

// Tests of CInterfileImageLayout: the image data described by the headers of the
// Interfile variants (Interfile 3.3 static, tomographic and dynamic, the PET proposal
// as written by STIR, the spellings of Siemens and the keys of PMOD), with synthetic
// headers and with the Interfile test datasets, whose expected values
// (testsuite/data/interfile/*.json) are written by an own parser of
// testsuite/data/tools/make_expected.py.

#include <QtTest/QtTest>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QtEndian>

#include <cmath>

#include "CInterfileImageLayout.h"
#include "CInterfileKeys.h"

#include "MedIOTestData.h"

Q_DECLARE_METATYPE(MedIOTestFile)

class TestInterfileLayout : public QObject
{
  Q_OBJECT

  private slots:
    void tomographic33();
    void tomographicSlices();
    void dynamicFrameGroups();
    void stirFrames();
    void siemensSpellings();
    void siemensImageHeader();
    void siemensMainHeader();
    void quantificationFactor();
    void dataFileNames();
    void invalidLayouts_data();
    void invalidLayouts();

    void datasets_data();
    void datasets();
};

// the layout of a header given as text (header file "dir/image.hdr")
static CInterfileImageLayout layoutOf(const QByteArray& text, bool* ok = 0, QString* error = 0,
                                      const QString& headerFile = "dir/image.hdr")
{
  CInterfileKeys keys;
  CInterfileImageLayout layout;
  QString reason;
  bool success = keys.parse("!INTERFILE :=\n" + text + "!END OF INTERFILE :=\n", &reason) &&
                 layout.read(keys, headerFile, &reason);
  if(ok)
    *ok = success;
  else if(success == false)
    qWarning("%s", qPrintable(reason));
  if(error)
    *error = reason;
  return layout;
}

static QVector<int> dims(int x, int y, int z)
{
  QVector<int> result;
  result << x << y << z;
  return result;
}

static QVector<double> vec(double x, double y, double z)
{
  QVector<double> result;
  result << x << y << z;
  return result;
}

void TestInterfileLayout::tomographic33()
{
  // Interfile 3.3 tomographic data as written by PMOD (header/image pair)
  bool ok = false;
  const CInterfileImageLayout layout = layoutOf(
      "!version of keys := 3.3\n"
      "!data offset in bytes := 0\n"
      "!name of data file := image.img\n"
      "!type of data := Tomographic\n"
      "!imagedata byte order := LITTLEENDIAN\n"
      "!SPECT STUDY (general) :=\n"
      "!matrix size [1] := 33\n"
      "!matrix size [2] := 32\n"
      "!matrix size [3] := 37\n"
      "!number format := signed integer\n"
      "!number of bytes per pixel := 2\n"
      "scaling factor (mm/pixel) [1] := 1.65\n"
      "scaling factor (mm/pixel) [2] := 1.75\n"
      "scaling factor (mm/pixel) [3] := 1.646\n"
      "image relative start time (sec) := 10.0\n"
      "image duration (sec) := 1800.0\n"
      "units := BQML\n"
      "origin (mm) [1] := -1.5\n"
      "origin (mm) [2] := 2.5\n"
      "origin (mm) [3] := 0.0\n"
      "data rescale offset := 0.5\n"
      "data rescale slope := 1.68\n", &ok);
  QVERIFY(ok);

  QCOMPARE(layout.versionOfKeys, QString("3.3"));
  QCOMPARE(layout.typeOfData, QString("tomographic"));
  QCOMPARE(layout.dataFileName, QString("dir/image.img"));
  QVERIFY(layout.dataInHeaderFile == false);
  QVERIFY(layout.bigEndian == false);
  QCOMPARE(layout.frames.size(), 1);

  const CInterfileFrame& frame = layout.frames.first();
  QCOMPARE(frame.numberFormat, CInterfileFrame::SignedInteger);
  QCOMPARE(frame.bytesPerPixel, 2);
  QCOMPARE(frame.dims, dims(33, 32, 37));
  QCOMPARE(frame.voxelSize, vec(1.65, 1.75, 1.646));
  QVERIFY(frame.hasOrigin);
  QCOMPARE(frame.origin, vec(-1.5, 2.5, 0.0));
  QCOMPARE(frame.startTime, 10.0);
  QCOMPARE(frame.duration, 1800.0);
  QCOMPARE(frame.slope, 1.68);
  QCOMPARE(frame.intercept, 0.5);
  QCOMPARE(frame.units, QString("BQML"));
  QCOMPARE(frame.offset, qint64(0));
  QCOMPARE(frame.byteSize(), qint64(33 * 32 * 37 * 2));
  QCOMPARE(layout.requiredDataSize(), qint64(33 * 32 * 37 * 2));
}

void TestInterfileLayout::tomographicSlices()
{
  // Interfile 3.3 without "matrix size [3]": planes from "number of slices", plane
  // distance from "slice thickness (pixels)"; big endian and unsigned if not given
  bool ok = false;
  const CInterfileImageLayout layout = layoutOf(
      "!name of data file := image.img\n"
      "!matrix size [1] := 64\n"
      "!matrix size [2] := 64\n"
      "!number of bytes per pixel := 2\n"
      "scaling factor (mm/pixel) [1] := 4.0\n"
      "scaling factor (mm/pixel) [2] := 4.0\n"
      "!number of slices := 20\n"
      "slice thickness (pixels) := 1.5\n", &ok);
  QVERIFY(ok);

  QVERIFY(layout.bigEndian);
  QCOMPARE(layout.frames.size(), 1);
  const CInterfileFrame& frame = layout.frames.first();
  QCOMPARE(frame.numberFormat, CInterfileFrame::UnsignedInteger);
  QCOMPARE(frame.dims, dims(64, 64, 20));
  QCOMPARE(frame.voxelSize, vec(4.0, 4.0, 6.0));
  QVERIFY(frame.hasOrigin == false);
  QCOMPARE(frame.duration, 0.0);
  QCOMPARE(frame.slope, 1.0);
}

void TestInterfileLayout::dynamicFrameGroups()
{
  // Interfile 3.3 dynamic study in a single file: frame groups with different
  // durations and number formats, pauses between images and between groups
  bool ok = false;
  const CInterfileImageLayout layout = layoutOf(
      "!data starting block := 1\n"
      "!name of data file :=\n"
      "!type of data := Dynamic\n"
      "!imagedata byte order := LITTLEENDIAN\n"
      "!number of frame groups := 2\n"
      "!Dynamic Study (each frame group) :=\n"
      "!frame group number := 1\n"
      "!matrix size [1] := 4\n"
      "!matrix size [2] := 3\n"
      "!matrix size [3] := 2\n"
      "!number format := signed integer\n"
      "!number of bytes per pixel := 2\n"
      "scaling factor (mm/pixel) [1] := 2\n"
      "scaling factor (mm/pixel) [2] := 2\n"
      "scaling factor (mm/pixel) [3] := 3\n"
      "!number of images this frame group := 4\n"
      "!image duration (sec) := 10\n"
      "pause between images (sec) := 5\n"
      "pause between frame groups (sec) := 20\n"
      "data rescale slope := 0.5\n"
      "units := kBq/cc\n"
      "!Dynamic Study (each frame group) :=\n"
      "!frame group number := 2\n"
      "!matrix size [1] := 4\n"
      "!matrix size [2] := 3\n"
      "!matrix size [3] := 2\n"
      "!number format := short float\n"
      "!number of images this frame group := 2\n"
      "!image duration (sec) := 60\n"
      "data rescale slope := 0.25\n", &ok);
  QVERIFY(ok);

  QVERIFY(layout.dataInHeaderFile);
  QCOMPARE(layout.dataFileName, QString("dir/image.hdr"));
  QCOMPARE(layout.frames.size(), 3);

  // group 1: two frames of 2 planes, int16
  QCOMPARE(layout.frames[0].numberFormat, CInterfileFrame::SignedInteger);
  QCOMPARE(layout.frames[0].offset, qint64(2048));
  QCOMPARE(layout.frames[0].startTime, 0.0);
  QCOMPARE(layout.frames[0].duration, 10.0);
  QCOMPARE(layout.frames[0].slope, 0.5);
  QCOMPARE(layout.frames[0].units, QString("kBq/cc"));
  QCOMPARE(layout.frames[0].voxelSize, vec(2.0, 2.0, 3.0));
  QCOMPARE(layout.frames[1].offset, qint64(2048 + 24 * 2));
  QCOMPARE(layout.frames[1].startTime, 15.0);

  // group 2: after the pause between the groups, float32, the keys not given in the
  // group are not taken from group 1 (voxel size 1 mm, no units)
  QCOMPARE(layout.frames[2].numberFormat, CInterfileFrame::Float);
  QCOMPARE(layout.frames[2].bytesPerPixel, 4);
  QCOMPARE(layout.frames[2].offset, qint64(2048 + 2 * 24 * 2));
  QCOMPARE(layout.frames[2].startTime, 45.0);
  QCOMPARE(layout.frames[2].duration, 60.0);
  QCOMPARE(layout.frames[2].slope, 0.25);
  QCOMPARE(layout.frames[2].voxelSize, vec(1.0, 1.0, 1.0));
  QVERIFY(layout.frames[2].units.isEmpty());
  QCOMPARE(layout.requiredDataSize(), qint64(2048 + 2 * 24 * 2 + 24 * 4));
}

void TestInterfileLayout::stirFrames()
{
  // PET proposal as written by STIR: vectored keys per frame, own data offsets
  bool ok = false;
  const CInterfileImageLayout layout = layoutOf(
      "!imaging modality := PT\n"
      "!version of keys := STIR6.0\n"
      "name of data file := image.v\n"
      "!type of data := PET\n"
      "imagedata byte order := LITTLEENDIAN\n"
      "!PET STUDY (General) :=\n"
      "!PET data type := Image\n"
      "process status := Reconstructed\n"
      "!number format := float\n"
      "!number of bytes per pixel := 4\n"
      "number of dimensions := 3\n"
      "matrix axis label [1] := x\n"
      "!matrix size [1] := 5\n"
      "scaling factor (mm/pixel) [1] := 2.08626\n"
      "matrix axis label [2] := y\n"
      "!matrix size [2] := 4\n"
      "scaling factor (mm/pixel) [2] := 2.08626\n"
      "matrix axis label [3] := z\n"
      "!matrix size [3] := 3\n"
      "scaling factor (mm/pixel) [3] := 2.03125\n"
      "first pixel offset (mm) [1] := -4.17252\n"
      "first pixel offset (mm) [2] := -4.17252\n"
      "first pixel offset (mm) [3] := 0\n"
      "number of time frames := 2\n"
      "image duration (sec)[1] := 30\n"
      "image relative start time (sec)[1] := 0\n"
      "image duration (sec)[2] := 60\n"
      "image relative start time (sec)[2] := 40\n"
      "image scaling factor[1] := 2\n"
      "data offset in bytes[1] := 1000\n"
      "image scaling factor[2] := 3\n"
      "data offset in bytes[2] := 100\n"
      "quantification units := 2\n", &ok);
  QVERIFY(ok);

  QCOMPARE(layout.versionOfKeys, QString("STIR6.0"));
  QCOMPARE(layout.typeOfData, QString("pet"));
  QCOMPARE(layout.dataFileName, QString("dir/image.v"));
  QCOMPARE(layout.frames.size(), 2);

  QCOMPARE(layout.frames[0].numberFormat, CInterfileFrame::Float);
  QCOMPARE(layout.frames[0].dims, dims(5, 4, 3));
  QCOMPARE(layout.frames[0].origin, vec(-4.17252, -4.17252, 0.0));
  QCOMPARE(layout.frames[0].offset, qint64(1000));
  QCOMPARE(layout.frames[0].startTime, 0.0);
  QCOMPARE(layout.frames[0].duration, 30.0);
  // "quantification units" equal to the image scaling factor is not applied twice
  QCOMPARE(layout.frames[0].slope, 2.0);
  QVERIFY(layout.frames[0].units.isEmpty());

  QCOMPARE(layout.frames[1].offset, qint64(100));
  QCOMPARE(layout.frames[1].startTime, 40.0);
  QCOMPARE(layout.frames[1].duration, 60.0);
  QCOMPARE(layout.frames[1].slope, 3.0);
  QCOMPARE(layout.requiredDataSize(), qint64(1000 + 60 * 4));
}

void TestInterfileLayout::siemensSpellings()
{
  // the spellings of Siemens: "image data byte order", "scale factor (mm/pixel)",
  // vendor keys; units in "quantification units"; contiguous frames
  bool ok = false;
  const CInterfileImageLayout layout = layoutOf(
      "%SMS-MI header name space := image subheader\n"
      "!imaging modality := PT\n"
      "!version of keys := 3.3\n"
      "!name of data file := image.v\n"
      "image data byte order := LITTLEENDIAN\n"
      "%patient orientation := HFS\n"
      "!PET data type := image\n"
      "number format := float\n"
      "!number of bytes per pixel := 4\n"
      "number of dimensions := 3\n"
      "matrix size [1] := 6\n"
      "matrix size [2] := 6\n"
      "matrix size [3] := 2\n"
      "scale factor (mm/pixel) [1] := 1.65\n"
      "scale factor (mm/pixel) [2] := 1.65\n"
      "scale factor (mm/pixel) [3] := 1.6456\n"
      "number of time frames := 2\n"
      "image duration (sec)[1] := 120\n"
      "image duration (sec)[2] := 120\n"
      "quantification units := Bq/ml\n", &ok);
  QVERIFY(ok);

  QVERIFY(layout.bigEndian == false);
  QCOMPARE(layout.frames.size(), 2);
  QCOMPARE(layout.frames[0].voxelSize, vec(1.65, 1.65, 1.6456));
  QCOMPARE(layout.frames[0].units, QString("Bq/ml"));
  QCOMPARE(layout.frames[0].slope, 1.0);
  QCOMPARE(layout.frames[1].offset, qint64(6 * 6 * 2 * 4));
  QCOMPARE(layout.frames[1].startTime, 120.0);
}

void TestInterfileLayout::siemensImageHeader()
{
  // the image header (.v.hdr) of a Siemens reconstruction (e7tools), shortened, as
  // written: "key:=value", no "!END OF INTERFILE", the data file named after the
  // image data description; %image slope/%image intercept scale the values
  CInterfileKeys keys;
  QString error;
  QVERIFY2(keys.parse(
      "!INTERFILE:=\r\n"
      "!originating system:=1208\r\n"
      "%SMS-MI header name space:=image subheader\r\n"
      "%SMS-MI version number:=3.9\r\n"
      "\r\n"
      "!GENERAL DATA:=\r\n"
      "\r\n"
      "!GENERAL IMAGE DATA:=\r\n"
      "image data byte order:=LITTLEENDIAN\r\n"
      "%study date (yyyy:mm:dd):=2026:11:13\r\n"
      "isotope name:=F-18\r\n"
      "!PET data type:=image\r\n"
      "number format:=float\r\n"
      "!number of bytes per pixel:=4\r\n"
      "number of dimensions:=3\r\n"
      "matrix axis label[1]:=x\r\n"
      "matrix axis label[2]:=y\r\n"
      "matrix axis label[3]:=z\r\n"
      "matrix size[1]:=32\r\n"
      "matrix size[2]:=32\r\n"
      "matrix size[3]:=583\r\n"
      "scale factor (mm/pixel) [1]:=22.6875\r\n"
      "scale factor (mm/pixel) [2]:=22.6875\r\n"
      "scale factor (mm/pixel) [3]:=1.64557\r\n"
      "start horizontal bed position (mm):=-718.542\r\n"
      "%energy window lower level (keV) [1]:=435\r\n"
      "\r\n"
      "!IMAGE DATA DESCRIPTION:=\r\n"
      "!image relative start time (sec):=0.0\r\n"
      "!image duration (sec):=435\r\n"
      "%scanner quantification factor (Bq*s/ECAT counts):=7.83898e+007\r\n"
      "quantification units:=Bq/ml\r\n"
      "!name of data file:=image_000_000.v\r\n"
      "%image orientation:={1,0,0,0,1,0}\r\n"
      "%patient orientation:=HFS\r\n"
      "%image slope:=2\r\n"
      "%image intercept:=0.5\r\n"
      "minimum pixel count:=-64.3102\r\n"
      "maximum pixel count:=84826.2\r\n"
      "%image info[1]:={0,-718.542,-362.064,-574.907,-1.19881,121.367,3809.89,1,0.0,1.00311,37.7978}\r\n",
      &error), qPrintable(error));

  CInterfileImageLayout layout;
  QVERIFY2(layout.read(keys, "dir/image_000_000.v.hdr", &error), qPrintable(error));
  QCOMPARE(layout.dataFileName, QString("dir/image_000_000.v"));
  QVERIFY(layout.bigEndian == false);
  QCOMPARE(layout.frames.size(), 1);

  const CInterfileFrame& frame = layout.frames.first();
  QCOMPARE(frame.numberFormat, CInterfileFrame::Float);
  QCOMPARE(frame.bytesPerPixel, 4);
  QCOMPARE(frame.dims, dims(32, 32, 583));
  QCOMPARE(frame.voxelSize, vec(22.6875, 22.6875, 1.64557));
  QCOMPARE(frame.startTime, 0.0);
  QCOMPARE(frame.duration, 435.0);
  QCOMPARE(frame.units, QString("Bq/ml"));
  QCOMPARE(frame.slope, 2.0);
  QCOMPARE(frame.intercept, 0.5);
  QCOMPARE(frame.offset, qint64(0));
  QCOMPARE(layout.requiredDataSize(), qint64(32 * 32 * 583 * 4));
}

void TestInterfileLayout::siemensMainHeader()
{
  // a Siemens main header (.mhdr) describes no image data, but its data sets
  CInterfileKeys keys;
  QVERIFY(keys.parse("!INTERFILE:=\r\n"
                     "%SMS-MI header name space:=image main header\r\n"
                     "number of time frames:=1\r\n"
                     "!total number of data sets:=1\r\n"
                     "%data set[1]:={0,image_000_000.v.hdr,image_000_000.v}\r\n"));

  CInterfileImageLayout layout;
  QString error;
  QVERIFY(layout.read(keys, "dir/image.mhdr", &error) == false);
  QVERIFY2(error.contains("main header of data sets"), qPrintable(error));
  QVERIFY2(error.contains("image_000_000.v.hdr"), qPrintable(error));
}

void TestInterfileLayout::quantificationFactor()
{
  // "quantification units" as factor without "image scaling factor" (Louvain-la-Neuve)
  bool ok = false;
  const CInterfileImageLayout layout = layoutOf(
      "!name of data file := image.v\n"
      "!number format := signed integer\n"
      "!number of bytes per pixel := 2\n"
      "!matrix size [1] := 2\n"
      "!matrix size [2] := 2\n"
      "quantification units := 0.125\n", &ok);
  QVERIFY(ok);
  QCOMPARE(layout.frames.first().slope, 0.125);
  QCOMPARE(layout.frames.first().dims, dims(2, 2, 1));
}

void TestInterfileLayout::dataFileNames()
{
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString header = dir.filePath("image.hv");
  QFile data(dir.filePath("image.v"));
  QVERIFY(data.open(QIODevice::WriteOnly));
  data.close();

  const QByteArray image = "!matrix size [1] := 2\n!matrix size [2] := 2\n!number of bytes per pixel := 1\n";

  // relative to the directory of the header
  bool ok = false;
  CInterfileImageLayout layout = layoutOf("!name of data file := image.v\n" + image, &ok, 0, header);
  QVERIFY(ok);
  QCOMPARE(QFileInfo(layout.dataFileName).absoluteFilePath(), QFileInfo(data).absoluteFilePath());

  // a path of another machine: the file of that name next to the header
  layout = layoutOf("!name of data file := C:\\recon\\data\\image.v\n" + image, &ok, 0, header);
  QVERIFY(ok);
  QCOMPARE(QFileInfo(layout.dataFileName).absoluteFilePath(), QFileInfo(data).absoluteFilePath());
  layout = layoutOf("!name of data file := /scanner/data/image.v\n" + image, &ok, 0, header);
  QVERIFY(ok);
  QCOMPARE(QFileInfo(layout.dataFileName).absoluteFilePath(), QFileInfo(data).absoluteFilePath());

  // no such file anywhere: the name as given (reading it fails later)
  layout = layoutOf("!name of data file := missing.v\n" + image, &ok, 0, header);
  QVERIFY(ok);
  QCOMPARE(layout.dataFileName, dir.filePath("missing.v"));
}

void TestInterfileLayout::invalidLayouts_data()
{
  QTest::addColumn<QByteArray>("text");
  QTest::addColumn<QString>("message");

  const QByteArray image = "!name of data file := image.img\n!matrix size [1] := 4\n!matrix size [2] := 4\n";
  const QByteArray bytes = "!number of bytes per pixel := 2\n";

  QTest::newRow("bit") << image + "!number format := bit\n" << "number format 'bit'";
  QTest::newRow("ascii") << image + "!number format := ASCII\n" << "number format 'ascii'";
  QTest::newRow("3 bytes") << image + "!number format := signed integer\n!number of bytes per pixel := 3\n"
                           << "3 bytes per pixel";
  QTest::newRow("2 byte float") << image + "!number format := float\n" + bytes << "2 bytes per pixel";
  QTest::newRow("no bytes") << image + "!number format := signed integer\n" << "number of bytes per pixel";
  QTest::newRow("no matrix size [2]") << QByteArray("!name of data file := image.img\n!matrix size [1] := 4\n") + bytes
                                      << "matrix size [2]";
  QTest::newRow("matrix size 0") << image + bytes + "!matrix size [3] := 0\n" << "matrix size [3] 0";
  QTest::newRow("matrix size list") << image + bytes + "!matrix size [3] := {95, 85}\n" << "different values per plane";
  QTest::newRow("4 dimensions") << image + bytes + "!matrix size [3] := 2\n!matrix size [4] := 5\n"
                                << "more than 3 dimensions";
  QTest::newRow("byte order") << image + bytes + "imagedata byte order := MIDDLEENDIAN\n" << "byte order";
  QTest::newRow("no offset in single file") << QByteArray("!matrix size [1] := 4\n!matrix size [2] := 4\n") + bytes
                                            << "no \"data offset in bytes\"";
  QTest::newRow("frames 0") << image + bytes + "number of time frames := 0\n" << "number of time frames 0";
  QTest::newRow("no number") << image + bytes + "scaling factor (mm/pixel) [1] := wide\n" << "is no number";
  QTest::newRow("images of group") << image + bytes + "!matrix size [3] := 3\n!frame group number := 1\n"
                                                      "!number of images this frame group := 4\n"
                                   << "4 images in a frame group of 3 planes";
}

void TestInterfileLayout::invalidLayouts()
{
  QFETCH(QByteArray, text);
  QFETCH(QString, message);

  bool ok = true;
  QString error;
  const CInterfileImageLayout layout = layoutOf(text, &ok, &error);
  QVERIFY(ok == false);
  QVERIFY2(error.contains(message), qPrintable(error));
  QVERIFY(layout.frames.isEmpty());
}

//-----------------------------------------------------------------------------------------
// the Interfile test datasets

void TestInterfileLayout::datasets_data()
{
  QTest::addColumn<MedIOTestFile>("file");

  const MedIOTestData data = medioTestData("interfile");
  foreach(const MedIOTestFile& file, data.files)
    QTest::newRow(file.relativePath.toLatin1().constData()) << file;

  if(data.files.isEmpty())
    QTest::newRow("no files") << MedIOTestFile();
}

// the raw values of a frame: minimum, maximum and sum
static bool rawStatistics(const CInterfileImageLayout& layout, const CInterfileFrame& frame,
                          double* minimum, double* maximum, double* sum)
{
  QFile file(layout.dataFileName);
  if(file.open(QIODevice::ReadOnly) == false || file.seek(frame.offset) == false)
    return false;
  const QByteArray data = file.read(frame.byteSize());
  if(data.size() != frame.byteSize())
    return false;

  const uchar* p = reinterpret_cast<const uchar*>(data.constData());
  *minimum = HUGE_VAL;
  *maximum = -HUGE_VAL;
  *sum = 0.0;
  for(qint64 i = 0; i < frame.voxelCount(); i++, p += frame.bytesPerPixel)
  {
    double value = 0.0;
    if(frame.numberFormat == CInterfileFrame::Float)
    {
      quint32 bits = layout.bigEndian ? qFromBigEndian<quint32>(p) : qFromLittleEndian<quint32>(p);
      float f = 0.0f;
      memcpy(&f, &bits, sizeof(f));
      value = f;
    }
    else if(frame.bytesPerPixel == 2)
    {
      const quint16 bits = layout.bigEndian ? qFromBigEndian<quint16>(p) : qFromLittleEndian<quint16>(p);
      value = (frame.numberFormat == CInterfileFrame::SignedInteger) ? double(qint16(bits)) : double(bits);
    }
    else
      return false;   // not in the test data
    *minimum = qMin(*minimum, value);
    *maximum = qMax(*maximum, value);
    *sum += value;
  }

  return true;
}

void TestInterfileLayout::datasets()
{
  QFETCH(MedIOTestFile, file);
  if(file.path.isEmpty())
    QSKIP("no Interfile test datasets");

  const QJsonObject header = file.expected.value("header").toObject();

  // the header: the file itself, or the .hdr file of a header/image pair directory
  // (Siemens: the image header of the data set of the main header .mhdr)
  QString headerFile = file.path;
  CInterfileKeys keys;
  CInterfileImageLayout layout;
  QString error;
  if(QFileInfo(file.path).isDir())
  {
    const QStringList headers = QDir(file.path).entryList(QStringList() << "*.hdr", QDir::Files);
    QCOMPARE(headers.size(), 1);
    headerFile = QDir(file.path).filePath(headers.first());

    const QStringList mainHeaders = QDir(file.path).entryList(QStringList() << "*.mhdr", QDir::Files);
    QCOMPARE(mainHeaders.isEmpty(), header.contains("data_sets") == false);
    if(mainHeaders.isEmpty() == false)
    {
      // the main header names the image header and data file of its data set, and
      // describes no image data itself
      QCOMPARE(mainHeaders.size(), 1);
      const QString mainHeader = QDir(file.path).filePath(mainHeaders.first());
      QVERIFY(CInterfileKeys::isInterfile(mainHeader));
      QVERIFY2(keys.read(mainHeader, &error), qPrintable(error));
      const QJsonArray dataSets = header.value("data_sets").toArray();
      QCOMPARE(dataSets.size(), 1);
      const QStringList dataSet = CInterfileKeys::toList(keys.value("data set", QList<int>() << 1));
      QCOMPARE(dataSet.size(), 3);
      QCOMPARE(dataSet.at(1), dataSets.at(0).toArray().at(0).toString());
      QCOMPARE(dataSet.at(2), dataSets.at(0).toArray().at(1).toString());
      QCOMPARE(QFileInfo(headerFile).fileName(), dataSet.at(1));
      QVERIFY(layout.read(keys, mainHeader, &error) == false);
      QVERIFY2(error.contains("main header of data sets"), qPrintable(error));
    }
  }
  QVERIFY(CInterfileKeys::isInterfile(headerFile));

  QVERIFY2(keys.read(headerFile, &error), qPrintable(error));
  QVERIFY2(layout.read(keys, headerFile, &error), qPrintable(error));

  QCOMPARE(layout.versionOfKeys, header.value("version_of_keys").toString());
  QCOMPARE(layout.typeOfData, header.value("type_of_data").toString().toLower());
  QCOMPARE(layout.bigEndian, header.value("byte_order").toString() == "big");
  QCOMPARE(layout.dataInHeaderFile, QFileInfo(file.path).isDir() == false);

  const QJsonArray frames = file.expected.value("frames").toArray();
  QCOMPARE(layout.frames.size(), frames.size());
  QCOMPARE(layout.frames.first().offset, qint64(header.value("data_offset").toDouble()));
  QVERIFY(QFileInfo(layout.dataFileName).size() >= layout.requiredDataSize());

  for(int f = 0; f < frames.size(); f++)
  {
    const QJsonObject expected = frames.at(f).toObject();
    const CInterfileFrame& frame = layout.frames.at(f);

    QJsonArray expectedDims = expected.value("dims").toArray();
    QCOMPARE(frame.dims, dims(expectedDims.at(0).toInt(), expectedDims.at(1).toInt(), expectedDims.at(2).toInt()));

    const QString format = expected.value("number_format").toString();
    const QString kind = (frame.numberFormat == CInterfileFrame::Float) ? "f"
                         : (frame.numberFormat == CInterfileFrame::SignedInteger) ? "i" : "u";
    QCOMPARE(kind + QString::number(frame.bytesPerPixel), format);

    const QJsonArray pixelSize = expected.value("pixel_size_mm").toArray();
    for(int i = 0; i < 3; i++)
      QCOMPARE(frame.voxelSize[i], pixelSize.at(i).toDouble());
    QCOMPARE(frame.duration, expected.value("duration_s").toDouble());
    if(expected.contains("start_s"))
      QCOMPARE(frame.startTime, expected.value("start_s").toDouble());
    QCOMPARE(frame.slope, expected.value("rescale_slope").toDouble());
    QCOMPARE(frame.intercept, expected.value("rescale_offset").toDouble());
    QCOMPARE(frame.units, expected.value("units").toString());

    // the stored values at the offset of the frame
    double minimum = 0.0;
    double maximum = 0.0;
    double sum = 0.0;
    QVERIFY(rawStatistics(layout, frame, &minimum, &maximum, &sum));
    QCOMPARE(minimum, expected.value("raw_min").toDouble());
    QCOMPARE(maximum, expected.value("raw_max").toDouble());
    QCOMPARE(sum, expected.value("raw_sum").toDouble());
  }

  // the frame timing of the ECAT7 file the Interfile file was converted from
  const QString ecat = QString("ecat/%1.v").arg(file.dataset);
  foreach(const MedIOTestFile& other, medioTestData("ecat").files)
  {
    if(other.relativePath != ecat)
      continue;
    const QJsonArray matrices = other.expected.value("matrices").toArray();
    QCOMPARE(layout.frames.size(), matrices.size());
    for(int f = 0; f < matrices.size(); f++)
    {
      const QJsonObject matrix = matrices.at(f).toObject();
      QCOMPARE(layout.frames.at(f).startTime * 1000.0, matrix.value("frame_start_time_ms").toDouble());
      QCOMPARE(layout.frames.at(f).duration * 1000.0, matrix.value("frame_duration_ms").toDouble());
    }
  }
}

QTEST_GUILESS_MAIN(TestInterfileLayout)
#include "testInterfileLayout.moc"
