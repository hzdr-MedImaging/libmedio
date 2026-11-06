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

#ifndef MEDIOTESTDATA_H
#define MEDIOTESTDATA_H

// Helper for the tests using the test data in testsuite/data (see
// testsuite/data/README.md). The data directory is passed by CMake via
// the MEDIO_TEST_DATA_DIR define.

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>

#ifndef MEDIO_TEST_DATA_DIR
#error "MEDIO_TEST_DATA_DIR has to be defined"
#endif

// a data file (or dataset directory) of testsuite/data/<format>/
struct MedIOTestFile
{
  QString dataset;          // dataset name (file name up to the first dot)
  QString format;           // format directory, e.g. "ecat"
  QString relativePath;     // path relative to the data directory, e.g. "ecat/pet-static.v"
  QString path;             // absolute path
  QJsonObject expected;     // content of <file>.json
  QJsonObject fileInfo;     // entry of the file in datasets.json
  QJsonObject datasetInfo;  // entry of the dataset in datasets.json
};

// content of the test data directory
struct MedIOTestData
{
  QList<MedIOTestFile> files;  // complete data files
  QStringList pending;         // files listed in datasets.json but not existing yet
  QStringList errors;          // inconsistencies (see testsuite/data/README.md)
};

// base directory of the test data
static inline QString medioTestDataDir()
{
  return QString(MEDIO_TEST_DATA_DIR);
}

static inline QJsonObject medioReadJson(const QString& fileName, QString* error)
{
  QFile file(fileName);
  error->clear();

  if(file.open(QIODevice::ReadOnly) == false)
  {
    *error = "missing";
    return QJsonObject();
  }

  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);

  if(parseError.error != QJsonParseError::NoError || doc.isObject() == false)
  {
    *error = "invalid JSON: " + parseError.errorString();
    return QJsonObject();
  }

  return doc.object();
}

// Returns the data files of the given format directory (all formats if
// format is empty) together with the pending files and all inconsistencies
// found. Files with inconsistencies are not part of the returned files.
static inline MedIOTestData medioTestData(const QString& format = QString())
{
  MedIOTestData result;
  QDir dataDir(medioTestDataDir());

  // datasets.json: file path -> (dataset name, file entry)
  QString error;
  QJsonObject datasets = medioReadJson(dataDir.filePath("datasets.json"), &error).value("datasets").toObject();
  QMap<QString, QString> listedDataset;

  if(error.isEmpty() == false)
    result.errors.append("datasets.json: " + error);

  foreach(const QString& name, datasets.keys())
  {
    QJsonObject files = datasets.value(name).toObject().value("files").toObject();

    if(datasets.value(name).toObject().value("description").toString().isEmpty())
      result.errors.append("datasets.json: dataset " + name + ": \"description\" missing");

    foreach(const QString& path, files.keys())
    {
      QString fileFormat = path.section('/', 0, 0);
      QString fileDataset = QFileInfo(path).fileName().section('.', 0, 0);

      if(format.isEmpty() == false && fileFormat != format)
        continue;

      if(fileDataset != name)
        result.errors.append("datasets.json: " + path + " is listed under dataset " + name +
                             " but its file name denotes dataset " + fileDataset);
      else if(QFileInfo(dataDir.filePath(path)).exists() == false)
        result.pending.append(path);

      listedDataset.insert(path, name);
    }
  }

  // data files of the format directories
  QStringList formats;
  if(format.isEmpty())
    formats = dataDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
  else
    formats << format;
  formats.removeAll("tools");

  foreach(const QString& fmt, formats)
  {
    QDir formatDir(dataDir.filePath(fmt));
    QSet<QString> dataNames;

    foreach(const QString& name, formatDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
    {
      if(name.endsWith(".json") || name == "PLACEHOLDER.md" || name == "README.md")
        continue;

      dataNames.insert(name);

      MedIOTestFile file;
      file.format = fmt;
      file.dataset = name.section('.', 0, 0);
      file.relativePath = fmt + "/" + name;
      file.path = formatDir.absoluteFilePath(name);

      if(listedDataset.value(file.relativePath) != file.dataset)
      {
        result.errors.append(file.relativePath + ": not listed under dataset " + file.dataset + " in datasets.json");
        continue;
      }

      file.expected = medioReadJson(file.path + ".json", &error);
      if(error.isEmpty() == false)
      {
        result.errors.append(file.relativePath + ".json: " + error + " (run tools/make_expected.py)");
        continue;
      }

      file.datasetInfo = datasets.value(file.dataset).toObject();
      file.fileInfo = file.datasetInfo.value("files").toObject().value(file.relativePath).toObject();
      result.files.append(file);
    }

    // expected values without data file
    foreach(const QString& name, formatDir.entryList(QStringList() << "*.json", QDir::Files, QDir::Name))
    {
      if(dataNames.contains(name.left(name.length() - 5)) == false)
        result.errors.append(fmt + "/" + name + ": data file " + name.left(name.length() - 5) + " missing");
    }
  }

  return result;
}

#endif // MEDIOTESTDATA_H
