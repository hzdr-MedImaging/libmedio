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

#ifndef MEDIOTESTTIMEZONE_H
#define MEDIOTESTTIMEZONE_H

// Dates and times without time zone are local time (see CMedIOHeaderMapping.h).
// Tests of them set a time zone of their own, so that they do not depend on
// the time zone of the machine: MedIOTestTimeZone sets TZ while it exists.

#include <QByteArray>
#include <QtGlobal>

#include <time.h>

class MedIOTestTimeZone
{
  public:
    explicit MedIOTestTimeZone(const char* zone)
      : m_set(qEnvironmentVariableIsSet("TZ")), m_previous(qgetenv("TZ"))
    {
      qputenv("TZ", zone);
      tzset();
    }

    ~MedIOTestTimeZone()
    {
      if(m_set)
        qputenv("TZ", m_previous);
      else
        qunsetenv("TZ");
      tzset();
    }

  private:
    Q_DISABLE_COPY(MedIOTestTimeZone)
    bool m_set;
    QByteArray m_previous;
};

#endif // MEDIOTESTTIMEZONE_H
