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

//! @file CHDF5File.cpp
//! @brief contains the implementation of the class CHDF5File

#include "CHDF5File.h"

#include <QByteArray>
#include <QFile>

#include <rtdebug.h>

#ifdef MEDIO_HAVE_HDF5
#include <hdf5.h>
#endif

namespace {

// the signature of an HDF5 file (format specification: "\211HDF\r\n\032\n")
const char signature[] = { '\x89', 'H', 'D', 'F', '\r', '\n', '\x1a', '\n' };
const qint64 signatureSize = sizeof(signature);

// the signature is at the start of the file or, after a user block, at offset
// 512, 1024, 2048, ... (the size of a user block is a power of two from 512)
const qint64 firstUserBlockSize = 512;

}

class CHDF5FilePrivate
{
  public:
    QString error;
};

CHDF5File::CHDF5File(const QString& fileName)
  : CMedIOData(fileName),
    m_pData(new CHDF5FilePrivate)
{
  ENTER();
  LEAVE();
}

CHDF5File::~CHDF5File()
{
  ENTER();

  close();
  delete m_pData;

  LEAVE();
}

bool CHDF5File::isOfType(const QString& fileName)
{
  ENTER();

  bool result = false;
  QFile file(fileName);
  if(file.open(QIODevice::ReadOnly))
  {
    const qint64 size = file.size();
    for(qint64 offset = 0; result == false && offset + signatureSize <= size;
        offset = (offset == 0) ? firstUserBlockSize : offset * 2)
    {
      result = file.seek(offset) && file.read(signatureSize) == QByteArray(signature, signatureSize);
    }
    file.close();
  }

  RETURN(result);
  return result;
}

CMedIOData* CHDF5File::createFromFile(const QString& fileName)
{
  ENTER();

  CMedIOData* result = isOfType(fileName) ? new CHDF5File(fileName) : NULL;

  RETURN(result);
  return result;
}

bool CHDF5File::hasSupport()
{
#ifdef MEDIO_HAVE_HDF5
  return true;
#else
  return false;
#endif
}

QString CHDF5File::libraryVersion()
{
  ENTER();

  QString result;
#ifdef MEDIO_HAVE_HDF5
  unsigned major = 0, minor = 0, release = 0;
  if(H5get_libversion(&major, &minor, &release) >= 0)
    result = QString("%1.%2.%3").arg(major).arg(minor).arg(release);
#endif

  LEAVE();
  return result;
}

bool CHDF5File::open(QIODevice::OpenModeFlag mode)
{
  ENTER();
  Q_UNUSED(mode);

  if(isOpen())
    close();

  if(hasSupport())
    m_pData->error = QString("HDF5 files cannot be read or written yet");
  else
    m_pData->error = QString("libmedio was built without HDF5 support");
  E("%s: %s", qPrintable(fileName()), qPrintable(m_pData->error));

  RETURN(false);
  return false;
}

void CHDF5File::close()
{
  ENTER();

  if(isOpen())
    QFile::close();

  LEAVE();
}

CMedIOData::Format CHDF5File::dataFormat() const
{
  return CMedIOData::HDF5;
}

QString CHDF5File::openError() const
{
  return m_pData->error;
}
