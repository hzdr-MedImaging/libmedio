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

#include "pmedio.h"

#include <CMedIO>

namespace py = nanobind;

// init function for the CMedIO class
void init_medio(py::module_ &m)
{
  // CMedIO
  py::class_<CMedIO>(m, "MedIO")
    .def_static("majorVersion", &CMedIO::majorVersion)
    .def_static("minorVersion", &CMedIO::minorVersion)
    .def_static("version", &CMedIO::version)
    .def_static("buildDate", &CMedIO::buildDate)
    .def_static("copyright", &CMedIO::copyright)
    .def_static("name", &CMedIO::name)
    .def_static("qtMajorVersion", &CMedIO::qtMajorVersion)
    .def_static("qtMinorVersion", &CMedIO::qtMinorVersion)
    .def_static("qtBuildVersion", &CMedIO::qtBuildVersion)
    .def_static("qtVersion", &CMedIO::qtVersion);
}
