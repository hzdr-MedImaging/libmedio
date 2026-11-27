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

// medioConvert - converts an image file of any format libmedio reads into any format it
// writes (ECAT7, NIfTI, Philips, Interfile, HDF5).
//
// The program is also installed under the names <source>2<target> (links to it, or
// copies where links are not available), e.g. ecat2nifti, ecat2hdf5, hdf52nifti or
// philips2ecat. The name the program is called with selects the conversion: the input
// has to be a file of the source format, and the output defaults to the input file name
// with the extension of the target format.
//
// ECAT7 and Philips files are converted into NIfTI and back by CNIFTIConverter (see
// CNIFTIConverter.h for the voxel values, geometry and metadata of the written file),
// with its options for NIfTI files. ecat2nifti/nifti2ecat and philips2nifti/nifti2philips
// were programs of their own before and keep their arguments and their output.
//
// All other conversions use the format independent interface of the library
// (CMedIOData), like pmedio.save() of the Python bindings and writeMedio() of the R
// package: the main header of the source in the representation of the target format,
// then every frame as volume.

#include <CHDF5File.h>
#include <CMedIOData.h>
#include <CMedIODataFactory.h>
#include <CMedIOHeader.h>
#include <CMedIOVolume.h>
#include <CNIFTIConverter.h>

#include <QByteArray>
#include <QFileInfo>
#include <QRegularExpression>
#include <QScopedPointer>
#include <QString>
#include <QStringList>

#include <iostream>

using namespace std;

namespace {

// the formats by the names used in the program names
struct FormatName
{
  const char* name;
  CMedIOData::Format format;
  const char* extension;   // of a new file, NULL: the format cannot be written
};

const FormatName formatNames[] = {
  { "ecat", CMedIOData::ECAT, ".v" },
  { "nifti", CMedIOData::NIFTI, ".nii" },
  { "philips", CMedIOData::Philips, ".img" },
  { "interfile", CMedIOData::Interfile, ".hv" },
  { "hdf5", CMedIOData::HDF5, ".h5" },
  { "micropet", CMedIOData::ConcordeMicropet, NULL }
};

const FormatName* formatOfName(const QString& name)
{
  for(unsigned i = 0; i < sizeof(formatNames) / sizeof(formatNames[0]); i++)
  {
    if(name == QLatin1String(formatNames[i].name))
      return &formatNames[i];
  }
  return NULL;
}

const FormatName* nameOfFormat(CMedIOData::Format format)
{
  for(unsigned i = 0; i < sizeof(formatNames) / sizeof(formatNames[0]); i++)
  {
    if(formatNames[i].format == format)
      return &formatNames[i];
  }
  return NULL;
}

QByteArray local(const QString& text)
{
  return text.toLocal8Bit();
}

// the name the program is called with, without directory, version suffix (-4.0.0)
// and extension (.exe)
QString programName(const char* argv0)
{
  QString name = QString::fromLocal8Bit(argv0);
  name = name.mid(qMax(name.lastIndexOf('/'), name.lastIndexOf('\\')) + 1).toLower();
  if(name.endsWith(".exe"))
    name.chop(4);
  name.remove(QRegularExpression("-[0-9]+(\\.[0-9a-z]+)*$"));
  return name;
}

void usage(const char* name)
{
  cout << "Usage: " << name << " [options] <input> <output>" << endl
       << endl
       << "Converts an image file of any format libmedio reads (ECAT7, NIfTI, Philips," << endl
       << "Interfile, microPET, HDF5) into another format." << endl
       << endl
       << "Options:" << endl
       << "  -f, --format <format>  format of the output: ecat, nifti, philips, interfile or" << endl
       << "                         hdf5. Default: the format of the output file name (.v," << endl
       << "                         .nii, .nii.gz, .hdr, .img, .syn, .hv, .h5, .hdf5, .hdf)" << endl
       << "  -h, --help             show this help" << endl
       << endl
       << "Options for ECAT7 and Philips files converted into NIfTI:" << endl
       << "  -2              write NIfTI-2 instead of NIfTI-1" << endl
       << "  --float         always write float32 values (Bq/ml), also for static data" << endl
       << "  --sidecar       also write the metadata as JSON sidecar file (<output>.json)" << endl
       << "  --no-extension  do not store the metadata as header extensions (DICOM, JSON)" << endl
       << endl
       << "An existing output file is replaced. HDF5 files can only be read and written if" << endl
       << "libmedio was built with HDF5 support"
       << (CHDF5File::hasSupport() ? " (this one was)." : " (this one was not).") << endl
       << endl
       << "Called as <source>2<target>, e.g. ecat2nifti, ecat2hdf5 or hdf52nifti, the program" << endl
       << "converts files of the source format into the target format, see" << endl
       << "<source>2<target> --help." << endl;
}

void usageOfAlias(const char* name, const FormatName& source, const FormatName& target)
{
  cout << "Usage: " << name << " <input> [output]" << endl
       << endl
       << "Converts an image file of the format " << source.name << " into the format " << target.name << "." << endl
       << "The output defaults to the input file name with the extension " << target.extension << "; an" << endl
       << "existing output file is replaced." << endl
       << endl
       << "The program is medioConvert under another name, see medioConvert --help." << endl;
}

// the file name without its extension (.nii.gz and the like: without both)
QString withoutExtension(const QString& fileName)
{
  QString name = fileName;
  if(name.endsWith(".gz", Qt::CaseInsensitive))
    name.chop(3);
  const int dot = name.lastIndexOf('.');
  if(dot > qMax(name.lastIndexOf('/'), name.lastIndexOf('\\')) + 1)
    name.truncate(dot);
  return name;
}

// an option of CNIFTIConverter for NIfTI files written from ECAT7 and Philips files;
// false if the argument is none
bool niftiOption(const QString& arg, CNIFTIConverter::Options& options)
{
  if(arg == "-2")
    options.niftiVersion = 2;
  else if(arg == "--float" || arg == "--normalize")
    options.floatData = true;
  else if(arg == "--sidecar")
    options.jsonSidecar = true;
  else if(arg == "--no-extension")
    options.jsonExtension = false;
  else
    return false;
  return true;
}

// ecat2nifti, philips2nifti
void usageToNifti(const char* name, bool philips)
{
  cout << "Usage: " << name << " [options] " << (philips ? "<input.img|input.syn>" : "<input.v>")
       << " [output.nii|.nii.gz|.hdr|.hdr.gz]" << endl
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
       << (philips ? "Called as nifti2philips, the program converts NIfTI into Philips files."
                   : "Called as nifti2ecat, the program converts NIfTI into ECAT7 files.") << endl
       << "The program is medioConvert under another name, see medioConvert --help." << endl;
}

int toNifti(int argc, char* argv[], bool philips)
{
  CNIFTIConverter::Options options;
  QString input;
  QString output;

  for(int i = 1; i < argc; i++)
  {
    const QString arg = QString::fromLocal8Bit(argv[i]);

    if(niftiOption(arg, options))
      continue;
    if(arg == "-h" || arg == "--help")
    {
      usageToNifti(argv[0], philips);
      return 0;
    }
    if(arg.startsWith("-"))
    {
      cerr << "Error: unknown option " << argv[i] << endl;
      usageToNifti(argv[0], philips);
      return 1;
    }
    if(input.isEmpty())
      input = arg;
    else if(output.isEmpty())
      output = arg;
    else
    {
      usageToNifti(argv[0], philips);
      return 1;
    }
  }

  if(input.isEmpty())
  {
    usageToNifti(argv[0], philips);
    return 1;
  }

  if(output.isEmpty())
  {
    output = input;
    const int dot = output.lastIndexOf('.');
    if(dot > output.lastIndexOf('/'))
      output.truncate(dot);
    output += ".nii";
  }

  QString error;
  const bool result = philips ? CNIFTIConverter::philipsToNifti(input, output, options, &error)
                              : CNIFTIConverter::ecatToNifti(input, output, options, &error);
  if(result == false)
  {
    cerr << "Error: " << local(error).constData() << endl;
    return 1;
  }

  cout << local(input).constData() << " -> " << local(output).constData() << endl;
  return 0;
}

// nifti2ecat, nifti2philips
void usageFromNifti(const char* name, bool philips)
{
  cout << "Usage: " << name << " <input.nii|.nii.gz|.hdr|.img|.hdr.gz|.img.gz> "
       << (philips ? "[output.img|output.syn]" : "[output.v]") << endl
       << endl
       << "The " << (philips ? "" : "ECAT7 ") << "metadata (patient, isotope, frame timing, ...) is restored from the" << endl
       << "JSON header extension of the input file or from a JSON sidecar file next to it;" << endl
       << "values missing there are taken from a DICOM header extension (e.g. the frame" << endl
       << "timing of PMOD) and, for files written by PMOD, from descrip." << endl;
  if(philips)
  {
    cout << "An output name ending with .syn writes a Syntegra file, any other name an image" << endl
         << "file. The output defaults to the input file name with the extension .img (for an" << endl
         << "input header/image pair: <name>.philips.img, so that the NIfTI image file is kept)." << endl;
  }
  else
    cout << "The output defaults to the input file name with the extension .v." << endl;
  cout << endl
       << (philips ? "Called as philips2nifti, the program converts Philips into NIfTI files."
                   : "Called as ecat2nifti, the program converts ECAT7 into NIfTI files.") << endl
       << "The program is medioConvert under another name, see medioConvert --help." << endl;
}

int fromNifti(int argc, char* argv[], bool philips)
{
  if(argc < 2 || argc > 3 || QString(argv[1]).startsWith("-"))
  {
    usageFromNifti(argv[0], philips);
    return (argc == 2 && (QString(argv[1]) == "-h" || QString(argv[1]) == "--help")) ? 0 : 1;
  }

  const QString input = QString::fromLocal8Bit(argv[1]);
  QString output = (argc == 3) ? QString::fromLocal8Bit(argv[2]) : QString();

  if(output.isEmpty())
  {
    output = input;
    if(output.endsWith(".gz", Qt::CaseInsensitive))
      output.chop(3);
    const int dot = output.lastIndexOf('.');
    const QString extension = (dot > output.lastIndexOf('/')) ? output.mid(dot).toLower() : QString();
    if(dot > output.lastIndexOf('/'))
      output.truncate(dot);
    // do not overwrite the image file of a NIfTI header/image pair
    if(philips)
      output += (extension == ".hdr" || extension == ".img") ? ".philips.img" : ".img";
    else
      output += ".v";
  }

  QString error;
  const bool result = philips ? CNIFTIConverter::niftiToPhilips(input, output, &error)
                              : CNIFTIConverter::niftiToEcat(input, output, &error);
  if(result == false)
  {
    cerr << "Error: " << local(error).constData() << endl;
    return 1;
  }

  cout << local(input).constData() << " -> " << local(output).constData() << endl;
  return 0;
}

// true if the two names are the same file (which does not have to exist)
bool sameFile(const QString& a, const QString& b)
{
  const QFileInfo first(a), second(b);
  if(first.exists() && second.exists())
    return first.canonicalFilePath() == second.canonicalFilePath();
  return first.absoluteFilePath() == second.absoluteFilePath();
}

// convert the file; false and the reason if that fails (no output file is left behind)
bool convert(const QString& input, const QString& output, CMedIOData::Format format, QString& error)
{
  if(QFileInfo::exists(input) == false)
  {
    error = QString("'%1' does not exist").arg(input);
    return false;
  }

  QScopedPointer<CMedIOData> in(CMedIODataFactory::create(input));
  if(in.isNull())
  {
    error = QString("'%1' is not a supported image file").arg(input);
    return false;
  }
  if(in->open(QIODevice::ReadOnly) == false)
  {
    CHDF5File* hdf5 = (in->dataFormat() == CMedIOData::HDF5) ? static_cast<CHDF5File*>(in.data()) : NULL;
    error = hdf5 ? QString("'%1' is an HDF5 file: %2").arg(input, hdf5->openError())
                 : QString("unable to open '%1'").arg(input);
    return false;
  }

  QScopedPointer<CMedIOData> out(CMedIODataFactory::createForWriting(output, format));
  if(out.isNull())
  {
    error = QString("files of the format of '%1' cannot be written").arg(output);
    return false;
  }

  // the output must not be one of the files of the input (e.g. the data file .v of an
  // Interfile header .hv)
  foreach(const QString& written, out->fileNames())
  {
    foreach(const QString& read, in->fileNames())
    {
      if(sameFile(written, read))
      {
        error = QString("the output '%1' is a file of the input").arg(written);
        return false;
      }
    }
  }

  const QList<short> frames = in->frameNumbers();
  if(frames.isEmpty())
  {
    error = QString("'%1' has no image frames").arg(input);
    return false;
  }

  // the main header of the input in the representation of the output format
  CMedIOHeader* header = NULL;
  if(in->readMainHeader(header, out->dataFormat()) == false || header == NULL)
  {
    delete header;
    error = QString("unable to convert the main header of '%1'").arg(input);
    return false;
  }
  const QScopedPointer<CMedIOHeader> mainHeader(header);

  CHDF5File* const hdf5 = (out->dataFormat() == CMedIOData::HDF5) ? static_cast<CHDF5File*>(out.data()) : NULL;
  if(out->open(QIODevice::WriteOnly) == false)
  {
    error = hdf5 ? QString("unable to create '%1': %2").arg(output, hdf5->openError())
                 : QString("unable to create '%1'").arg(output);
    return false;
  }

  bool result = out->writeMainHeader(*mainHeader);
  if(result == false)
    error = QString("unable to write the main header of '%1'").arg(output);

  // every frame as volume (values, positions of the voxels, frame timing), numbered 1..n
  for(int k = 0; result && k < frames.size(); k++)
  {
    CMedIOVolume volume;
    if(in->readVolume(volume, frames[k]) == false)
    {
      error = QString("unable to read frame %1 of '%2'").arg(frames[k]).arg(input);
      result = false;
    }
    else if(out->writeVolume(volume, static_cast<short>(k + 1)) == false)
    {
      error = QString("unable to write frame %1 into '%2'").arg(k + 1).arg(output);
      result = false;
    }
  }

  if(hdf5 != NULL)
  {
    // an HDF5 file gets its name when it is complete; otherwise there is no new file,
    // and an existing one is left as it is
    if(result && hdf5->finish() == false)
    {
      error = QString("unable to complete '%1': %2").arg(output, hdf5->openError());
      result = false;
    }
    hdf5->close();
  }
  else
  {
    out->close();
    if(result == false)
      out->removeFiles();
  }

  return result;
}

int run(const QString& input, const QString& output, CMedIOData::Format format)
{
  QString error;
  if(convert(input, output, format, error) == false)
  {
    cerr << "Error: " << local(error).constData() << endl;
    return 1;
  }

  cout << local(input).constData() << " -> " << local(output).constData() << endl;
  return 0;
}

// medioConvert: ECAT7 and Philips files into NIfTI and back as ecat2nifti, nifti2ecat,
// philips2nifti and nifti2philips convert them, everything else as volumes
int runAny(const QString& input, const QString& output, CMedIOData::Format format,
           const CNIFTIConverter::Options& options, bool hasOptions)
{
  const CMedIOData::Format source = QFileInfo::exists(input) ? CMedIODataFactory::identify(input) : CMedIOData::Unknown;
  const CMedIOData::Format target = (format != CMedIOData::Unknown) ? format : CMedIODataFactory::formatOfFileName(output);
  const bool toNifti = (target == CMedIOData::NIFTI && (source == CMedIOData::ECAT || source == CMedIOData::Philips));
  const bool fromNifti = (source == CMedIOData::NIFTI && (target == CMedIOData::ECAT || target == CMedIOData::Philips));

  if(hasOptions && toNifti == false)
  {
    cerr << "Error: the options -2, --float, --sidecar and --no-extension are for ECAT7 and "
         << "Philips files converted into NIfTI only" << endl;
    return 1;
  }
  if(toNifti == false && fromNifti == false)
    return run(input, output, format);

  QString error;
  bool result = false;
  if(sameFile(input, output))
    error = QString("the output '%1' is a file of the input").arg(output);
  else if(toNifti)
    result = (source == CMedIOData::ECAT) ? CNIFTIConverter::ecatToNifti(input, output, options, &error)
                                          : CNIFTIConverter::philipsToNifti(input, output, options, &error);
  else
    result = (target == CMedIOData::ECAT) ? CNIFTIConverter::niftiToEcat(input, output, &error)
                                          : CNIFTIConverter::niftiToPhilips(input, output, &error);
  if(result == false)
  {
    cerr << "Error: " << local(error).constData() << endl;
    return 1;
  }

  cout << local(input).constData() << " -> " << local(output).constData() << endl;
  return 0;
}

// called as <source>2<target>
int runAlias(int argc, char* argv[], const FormatName& source, const FormatName& target)
{
  QStringList files;
  for(int i = 1; i < argc; i++)
  {
    const QString arg = QString::fromLocal8Bit(argv[i]);
    if(arg == "-h" || arg == "--help")
    {
      usageOfAlias(argv[0], source, target);
      return 0;
    }
    if(arg.startsWith("-"))
    {
      cerr << "Error: unknown option " << argv[i] << endl;
      usageOfAlias(argv[0], source, target);
      return 1;
    }
    files << arg;
  }
  if(files.isEmpty() || files.size() > 2)
  {
    usageOfAlias(argv[0], source, target);
    return 1;
  }

  // the input has to be a file of the source format
  const QString input = files[0];
  if(QFileInfo::exists(input))
  {
    const CMedIOData::Format found = CMedIODataFactory::identify(input);
    if(found != source.format)
    {
      const FormatName* name = nameOfFormat(found);
      cerr << "Error: '" << local(input).constData() << "' is no " << source.name << " file";
      if(name != NULL)
        cerr << " (it is a " << name->name << " file)";
      cerr << endl;
      return 1;
    }
  }

  const QString output = (files.size() == 2) ? files[1] : withoutExtension(input) + target.extension;
  return run(input, output, target.format);
}

} // namespace

int main(int argc, char* argv[])
{
  // <source>2<target>: the conversion of the name
  const QString name = programName(argv[0]);
  const int separator = name.indexOf('2', 1);
  if(separator > 0)
  {
    // hdf52nifti: the first '2' after the name of the source
    const FormatName* source = formatOfName(name.left(separator));
    const FormatName* target = formatOfName(name.mid(separator + 1));
    if(source == NULL && name.startsWith("hdf52"))
    {
      source = formatOfName("hdf5");
      target = formatOfName(name.mid(5));
    }
    if(source != NULL && target != NULL && target->extension != NULL && source != target)
    {
      // the names of the former programs, with their arguments
      const bool philips = (source->format == CMedIOData::Philips || target->format == CMedIOData::Philips);
      if(target->format == CMedIOData::NIFTI && (source->format == CMedIOData::ECAT || philips))
        return toNifti(argc, argv, philips);
      if(source->format == CMedIOData::NIFTI && (target->format == CMedIOData::ECAT || philips))
        return fromNifti(argc, argv, philips);
      return runAlias(argc, argv, *source, *target);
    }
  }

  CMedIOData::Format format = CMedIOData::Unknown;
  CNIFTIConverter::Options options;
  bool hasOptions = false;
  QStringList files;
  for(int i = 1; i < argc; i++)
  {
    const QString arg = QString::fromLocal8Bit(argv[i]);
    if(niftiOption(arg, options))
    {
      hasOptions = true;
      continue;
    }
    if(arg == "-h" || arg == "--help")
    {
      usage(argv[0]);
      return 0;
    }
    if(arg == "-f" || arg == "--format" || arg.startsWith("--format="))
    {
      QString value = arg.startsWith("--format=") ? arg.mid(9) : QString();
      if(arg.startsWith("--format=") == false)
      {
        if(i + 1 >= argc)
        {
          cerr << "Error: " << argv[i] << " needs a format" << endl;
          return 1;
        }
        value = QString::fromLocal8Bit(argv[++i]);
      }
      const FormatName* given = formatOfName(value.toLower());
      if(given == NULL || given->extension == NULL)
      {
        cerr << "Error: '" << local(value).constData() << "' is no format to write (ecat, nifti, philips, "
             << "interfile, hdf5)" << endl;
        return 1;
      }
      format = given->format;
    }
    else if(arg.startsWith("-"))
    {
      cerr << "Error: unknown option " << argv[i] << endl;
      usage(argv[0]);
      return 1;
    }
    else
      files << arg;
  }

  if(files.size() != 2)
  {
    usage(argv[0]);
    return 1;
  }

  if(format == CMedIOData::Unknown && CMedIODataFactory::formatOfFileName(files[1]) == CMedIOData::Unknown)
  {
    cerr << "Error: unknown file format of '" << local(files[1]).constData()
         << "' (.v: ECAT7, .nii/.nii.gz/.hdr: NIfTI, .img/.syn: Philips, .hv: Interfile, "
         << ".h5/.hdf5/.hdf: HDF5); give it with --format" << endl;
    return 1;
  }

  return runAny(files[0], files[1], format, options, hasOptions);
}
