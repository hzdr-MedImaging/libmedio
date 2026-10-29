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

// Helper for the tests using the datasets in testsuite/data (see
// testsuite/data/README.md). The data directory is passed by CMake via
// the MEDIO_TEST_DATA_DIR define.

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

#ifndef MEDIO_TEST_DATA_DIR
#error "MEDIO_TEST_DATA_DIR has to be defined"
#endif

// a dataset of testsuite/data/<format>/<name>
struct MedIOTestDataset
{
  QString name;       // directory name of the dataset
  QString dir;        // absolute path of the dataset directory
  QJsonObject json;   // content of dataset.json

  // absolute path of a file given relative to the dataset directory
  QString filePath(const QString& relativePath) const
  {
    return QDir(dir).filePath(relativePath);
  }

  // absolute path of the data file of the dataset ("file" entry)
  QString dataFile() const
  {
    return filePath(json.value("file").toString());
  }
};

// base directory of the test data
static inline QString medioTestDataDir()
{
  return QString(MEDIO_TEST_DATA_DIR);
}

// Returns all datasets of the given format (sorted by name). Problems with
// a dataset (missing or invalid dataset.json, missing data file) are
// appended to errors and the dataset is skipped.
static inline QList<MedIOTestDataset> medioTestDatasets(const QString& format, QStringList* errors = NULL)
{
  QList<MedIOTestDataset> datasets;
  QDir formatDir(QDir(medioTestDataDir()).filePath(format));

  foreach(const QString& name, formatDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
  {
    MedIOTestDataset dataset;
    dataset.name = name;
    dataset.dir = formatDir.absoluteFilePath(name);

    QString error;
    QFile jsonFile(dataset.filePath("dataset.json"));

    if(jsonFile.open(QIODevice::ReadOnly) == false)
      error = "dataset.json missing";
    else
    {
      QJsonParseError parseError;
      QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);

      if(parseError.error != QJsonParseError::NoError || doc.isObject() == false)
        error = "dataset.json invalid: " + parseError.errorString();
      else
      {
        dataset.json = doc.object();

        if(dataset.json.value("file").toString().isEmpty())
          error = "dataset.json: \"file\" entry missing";
        else if(QFileInfo(dataset.dataFile()).isFile() == false)
          error = "data file " + dataset.json.value("file").toString() + " missing";
        else if(dataset.json.value("expected").isObject() == false)
          error = "dataset.json: \"expected\" block missing (run tools/make_expected.py)";
      }
    }

    if(error.isEmpty())
      datasets.append(dataset);
    else if(errors)
      errors->append(format + "/" + name + ": " + error);
  }

  return datasets;
}

#endif // MEDIOTESTDATA_H
