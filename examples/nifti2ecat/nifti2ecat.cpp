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

// nifti2ecat - converts a NIfTI-1 or NIfTI-2 file into an ECAT7 image file
//
// The conversion itself is done by CNIFTIConverter, see CNIFTIConverter.h
// for the voxel values, geometry and metadata of the written file.

#include <CNIFTIConverter.h>

#include <QString>

#include <iostream>

using namespace std;

static void usage(const char* name)
{
  cout << "Usage: " << name << " <input.nii|input.nii.gz|input.hdr> [output.v]" << endl
       << endl
       << "The ECAT7 metadata (patient, isotope, frame timing, ...) is restored from the" << endl
       << "JSON header extension of the input file or from a JSON sidecar file next to it." << endl
       << "The output defaults to the input file name with the extension .v." << endl;
}

int main(int argc, char* argv[])
{
  if(argc < 2 || argc > 3 || QString(argv[1]).startsWith("-")) {
    usage(argv[0]);
    return (argc == 2 && (QString(argv[1]) == "-h" || QString(argv[1]) == "--help")) ? 0 : 1;
  }

  const QString input = QString::fromLocal8Bit(argv[1]);
  QString output = (argc == 3) ? QString::fromLocal8Bit(argv[2]) : QString();

  if(output.isEmpty()) {
    output = input;
    if(output.endsWith(".gz", Qt::CaseInsensitive))
      output.chop(3);
    const int dot = output.lastIndexOf('.');
    if(dot > output.lastIndexOf('/'))
      output.truncate(dot);
    output += ".v";
  }

  QString error;
  if(CNIFTIConverter::niftiToEcat(input, output, &error) == false) {
    cerr << "Error: " << error.toLocal8Bit().constData() << endl;
    return 1;
  }

  cout << input.toLocal8Bit().constData() << " -> " << output.toLocal8Bit().constData() << endl;
  return 0;
}
