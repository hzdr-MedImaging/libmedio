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

// ecat2nifti and nifti2ecat - one program for both directions of the conversion between
// ECAT7 image files and NIfTI-1/NIfTI-2 files. The name the program is called with selects the
// direction: nifti2ecat (installed as a link to ecat2nifti, or as a copy where links are not
// available) converts NIfTI files into ECAT7 image files, any other name (ecat2nifti)
// converts ECAT7 image files into NIfTI files.
//
// The conversion itself is done by CNIFTIConverter, see CNIFTIConverter.h
// for the voxel values, geometry and metadata of the written file.

#include <CNIFTIConverter.h>

#include <QRegularExpression>
#include <QString>

#include <iostream>

using namespace std;

static void usageToNifti(const char* name)
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
       << "name ending with .hdr/.img (.hdr.gz/.img.gz) writes a NIfTI header/image pair." << endl
       << endl
       << "Called as nifti2ecat, the program converts NIfTI into ECAT7 files." << endl;
}

static int toNifti(int argc, char* argv[])
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
      usageToNifti(argv[0]);
      return 0;
    }
    else if(arg.startsWith("-")) {
      cerr << "Error: unknown option " << argv[i] << endl;
      usageToNifti(argv[0]);
      return 1;
    }
    else if(input.isEmpty())
      input = arg;
    else if(output.isEmpty())
      output = arg;
    else {
      usageToNifti(argv[0]);
      return 1;
    }
  }

  if(input.isEmpty()) {
    usageToNifti(argv[0]);
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

static void usageFromNifti(const char* name)
{
  cout << "Usage: " << name << " <input.nii|.nii.gz|.hdr|.img|.hdr.gz|.img.gz> [output.v]" << endl
       << endl
       << "The ECAT7 metadata (patient, isotope, frame timing, ...) is restored from the" << endl
       << "JSON header extension of the input file or from a JSON sidecar file next to it;" << endl
       << "values missing there are taken from a DICOM header extension (e.g. the frame" << endl
       << "timing of PMOD) and, for files written by PMOD, from descrip." << endl
       << "The output defaults to the input file name with the extension .v." << endl
       << endl
       << "Called as ecat2nifti, the program converts ECAT7 into NIfTI files." << endl;
}

static int fromNifti(int argc, char* argv[])
{
  if(argc < 2 || argc > 3 || QString(argv[1]).startsWith("-")) {
    usageFromNifti(argv[0]);
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

// the name the program is called with, without directory, version suffix (-4.0.0)
// and extension (.exe)
static QString programName(const char* argv0)
{
  QString name = QString::fromLocal8Bit(argv0);
  name = name.mid(qMax(name.lastIndexOf('/'), name.lastIndexOf('\\')) + 1).toLower();
  if(name.endsWith(".exe"))
    name.chop(4);
  name.remove(QRegularExpression("-[0-9]+(\\.[0-9a-z]+)*$"));
  return name;
}

int main(int argc, char* argv[])
{
  if(programName(argv[0]).startsWith("nifti2"))
    return fromNifti(argc, argv);

  return toNifti(argc, argv);
}
