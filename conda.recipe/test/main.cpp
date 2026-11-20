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

// A program using an installed libmedio (the public headers in include/medio and the
// library), as the test of the conda package (see recipe.yaml and CMakeLists.txt).

#include <CMedIOData>
#include <CMedIODataFactory>

#include <cstdio>

int main()
{
  // the format of a file name, without any file
  const CMedIOData::Format format = CMedIODataFactory::formatOfFileName("image.nii.gz");
  std::printf("libmedio: image.nii.gz is format %d\n", int(format));
  return format == CMedIOData::NIFTI ? 0 : 1;
}
