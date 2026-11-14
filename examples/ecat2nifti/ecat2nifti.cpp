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

// ecat2nifti - converts an ECAT7 image file into a NIfTI-1 or NIfTI-2 file
//
// The conversion itself is done by CNIFTIConverter, see CNIFTIConverter.h
// for the voxel values, geometry and metadata of the written file.

#include <CNIFTIConverter.h>

#include <QString>

#include <iostream>

using namespace std;

static void usage(const char* name)
{
  cout << "Usage: " << name << " [options] <input.v> [output.nii|.nii.gz|.hdr|.hdr.gz]" << endl
       << endl
       << "Options:" << endl
       << "  -2              write NIfTI-2 instead of NIfTI-1" << endl
       << "  --float         always write float32 values (Bq/ml), also for static data" << endl
       << "  --sidecar       also write the metadata as JSON sidecar file (<output>.json)" << endl
       << "  --no-extension  do not store the metadata as header extensions (DICOM, JSON)" << endl
       << endl
       << "The output defaults to the input file name with the extension .nii. An output" << endl
       << "name ending with .hdr/.img (.hdr.gz/.img.gz) writes a NIfTI header/image pair." << endl;
}

int main(int argc, char* argv[])
{
  CNIFTIConverter::Options options;
  QString input;
  QString output;

  for(int i=1; i < argc; i++) {
    const QString arg = QString::fromLocal8Bit(argv[i]);

    if(arg == "-2")
      options.niftiVersion = 2;
    else if(arg == "--float" || arg == "--normalize")
      options.floatData = true;
    else if(arg == "--sidecar")
      options.jsonSidecar = true;
    else if(arg == "--no-extension")
      options.jsonExtension = false;
    else if(arg == "-h" || arg == "--help") {
      usage(argv[0]);
      return 0;
    }
    else if(arg.startsWith("-")) {
      cerr << "Error: unknown option " << argv[i] << endl;
      usage(argv[0]);
      return 1;
    }
    else if(input.isEmpty())
      input = arg;
    else if(output.isEmpty())
      output = arg;
    else {
      usage(argv[0]);
      return 1;
    }
  }

  if(input.isEmpty()) {
    usage(argv[0]);
    return 1;
  }

  if(output.isEmpty()) {
    output = input;
    const int dot = output.lastIndexOf('.');
    if(dot > output.lastIndexOf('/'))
      output.truncate(dot);
    output += ".nii";
  }

  QString error;
  if(CNIFTIConverter::ecatToNifti(input, output, options, &error) == false) {
    cerr << "Error: " << error.toLocal8Bit().constData() << endl;
    return 1;
  }

  cout << input.toLocal8Bit().constData() << " -> " << output.toLocal8Bit().constData() << endl;
  return 0;
}
