#include "CApplication.h"
#include "CDataArray.h"

#include "config.h"

#include <CECATFile>
#include <CECAT7MainHeader>
#include <CECAT7SubHeaderImage>
#include <CPhilipsFile>
#include <CPhilipsMainHeader>
#include <CPhilipsSubHeaderImage>
#include <CMedIO>

#include <iostream>
#include <rtdebug.h>

#include <QDir>
#include <QString>
#include <QFileInfo>
#include <QMultiHash>
#include <QtCore/qmath.h>

#define NUMFRAMESLIMIT 511

using namespace std;

CApplication::CApplication()
 : m_bOverWrite(false),
   m_bPreserveDataType(false),
   m_bRecursive(false),
   m_bEcat2Img(false)
{
  ENTER();
  LEAVE();
}

bool CApplication::parseCmdLine(int argc, char* argv[])
{
  ENTER();
  bool bResult = false;

  SHOWVALUE(argc);

  // put all arguments in a temporary Hash
  QHash<QString, QString> args;

  // and all potential input filenames into a QStringList
  QStringList inputFileNames;

  // if the user has specified some commandline options
  // lets process and parse them.
  for(int i=1; i < argc; i++)
  {
    QString option(argv[i]);

    if(option[0] == '-')
    {
      QString nextOption(argv[i+1]);

      if(i+1 < argc && (nextOption[0] != '-' || nextOption[0].isLetter() == false))
        args.insert(option, nextOption);
      else
        args.insert(option, "");
    }
    else
      inputFileNames << argv[i];
  }

  // now we check/process the different options according to their
  // priority
  if(inputFileNames.isEmpty() == false &&
     args.contains("-h") == false &&
     args.contains("-v") == false)
  {
    bResult = true;

    if(args.contains("-f"))
    {
      m_bOverWrite = true;
    }

    if(args.contains("-o"))
    {
      m_sPreferedOutputFile = args.value("-o");
      inputFileNames.removeAll(args.value("-o"));
    }

#if 0
    if(args.contains("-n"))
    {
      m_sPatientName = args.value("-n");
      inputFileNames.removeAll(args.value("-n"));
    }

    if(args.contains("-p"))
    {
      m_bPreserveDataType = true;
    }
#endif

    if(args.contains("-r"))
    {
      m_bRecursive = true;
    }

    if(args.contains("-e") || QString(argv[0]).contains("ecat2img"))
    {
      m_bEcat2Img = true;
    }

    // we check if there is one and only one input file available
    // we do not support processing of multiple concorde micropet image files
    if(inputFileNames.isEmpty())
    {
      bResult = false;
      cout << "ERROR: no philips format image file provided" << endl;
    }
    else
      m_sInputFileNames = inputFileNames;
  }

  if(bResult == false)
    showAppInfo();

  if(args.contains("-v"))
    showVersion();
  else if(bResult == false)
    showUsage(argc, argv);

  RETURN(bResult);
  return bResult;
}

void CApplication::showUsage(int, char* argv[])
{
  ENTER();
  // output usage information on the console.
  cout << "Usage: " << argv[0] << " <options> <file.img>" << endl
       << "Options:" << endl
       << "  -o <file>    : ECAT image (*.v) to which the philips image is converted" << endl
       << "  -e           : reverse operation (create ECAT from img file)" << endl
//       << "  -p           : preserve data type as is and do not convert to short values" << endl
//       << "                 (this will give you the highest precision on cost of space)" << endl
       << "  -r           : walk the specified directory recursively and convert all *.img" << endl
       << "                 file in it in one run." << endl
       << "  -f           : force overwriting of existing files" << endl;
  LEAVE();
  return;
}

bool CApplication::checkOutputFile(const QFileInfo& inputFile)
{
  ENTER();
  bool bResult = true;
  QFileInfo outputFile;
  QString extension;

  if(m_bEcat2Img == false)
    extension = "v";
  else
    extension = "img";

  if(m_sPreferedOutputFile.isEmpty() == true)
    outputFile = QFileInfo(inputFile.filePath() + "." + extension);
  else
    outputFile = QFileInfo(m_sPreferedOutputFile);
    
  if(m_bOverWrite == false)
  {
    if(outputFile.exists())
    {
      bResult = false;
      cout << "ERROR: can't convert '" << inputFile.fileName().toAscii().constData() 
           << "' as output file '" << outputFile.fileName().toAscii().constData() << "' already exists" << endl;
    }
  }
  else
  {
    if(outputFile.exists() && outputFile.isFile() == false && outputFile.isWritable() == false)
    {
      bResult = false;
      cout << "ERROR: can not overwrite non-file" << endl;
    }
  }

  if(bResult)
  {
    // check if input file equals output file
    if(outputFile.absoluteFilePath() == inputFile.absoluteFilePath())
    {
      bResult = false;
      cout << "ERROR: will not overwrite input file" << endl;
    }
  }

  if(bResult)
    m_sOutputFileName = outputFile.absoluteFilePath();

  RETURN(bResult);
  return bResult;
}

void CApplication::showAppInfo()
{
  ENTER();
  // output some general program information
  cout << endl
       << PACKAGE_STRING << " - converts philips image files to ECAT files" << endl;
//       << "(" __DATE__ ") Copyright (c) 2006-2011 by Hagen Moelle, Jens Langner / www.hzdr.de" << endl << endl;
  LEAVE();
  return;
}
void CApplication::showVersion()
{
  ENTER();
  cout << "Detailed compilation information:" << endl << endl

       // Compiler information
       << "  "
       #if defined(__GNUC__)
       << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ <<  "." << __GNUC_PATCHLEVEL__ << " "
       #else
       #warning unknown compiler suite
       << "unknown compiler "
       #endif
       #if defined(__SPARC__)
       << "[sparc]"
       #elif defined(__POWERPC__)
       << "[ppc]"
       #elif defined(__i386__)
       << "[x86]"
       #elif defined(__X86_64__)
       << "[x86_64]"
       #else
       #warning Unknown CPU model
       << "[Unknown]"
       #endif
       << endl << endl
  
       // Qt version information
       << "  Qt " << qVersion() << endl
                  << "  Copyright (c) 2006-2011 Nokia Corporation" << endl << endl

       // libmedio version information
       << "  libmedio " << CMedIO::version().toAscii().constData() <<  " ("
       << CMedIO::buildDate().toAscii().constData() << ")" << endl
       << "  " << CMedIO::copyright().toAscii().constData() << endl;

  LEAVE();
  return;
}

bool CApplication::process()
{
  ENTER();
  bool result = true;

  // now we check out options and either convert bunch of files or a whole directory tree
  if(m_bRecursive == false)
  {
    QString fileName;
    foreach(fileName, m_sInputFileNames)
    {
      if(QFileInfo(fileName).exists() == true)
      {
        if(m_bEcat2Img == false)
          result = convert2Ecat(QFileInfo(fileName));
        else
          result = convert2Img(QFileInfo(fileName));

        if(result == false)
          break;
      }
      else
      {
        cout << "ERROR: file " << fileName.toAscii().constData()
             << " doesn't exist. Skipping file." << endl;
      }
    }
  }
  else
  {
    // lets walk the specified directory
    if(m_sInputFileNames.count() > 1)
    {
      cout << "ERROR: only one directory can be specified." << endl;
    }
    else
      result = walkDirectory(QDir(m_sInputFileNames[0]));
  }

  RETURN(result);
  return result;
}

bool CApplication::convert2Ecat(const QFileInfo& inputFile)
{
  ENTER();
  bool result = true;
  bool bUnknownDataType = false;
  CECATFile* pEcat7Image = NULL;
  CECAT7MainHeader* pEcat7ImageHeader = NULL;
  CPhilipsFile* pPhilipsFile = NULL;
  CPhilipsMainHeader* pPhilipsMainHeader = NULL;

  // check if we can create the output file
  if(checkOutputFile(inputFile) == false)
    result = false;

  if(result)
  {
    pPhilipsFile = new CPhilipsFile(inputFile.filePath());
    if(pPhilipsFile == NULL)
      result = false;
  }

  if(result)
  {
    bool bOpenPhilipsFile = pPhilipsFile->open(QIODevice::ReadOnly);
    if(bOpenPhilipsFile == false)
    {
      cout << "ERROR: Can't open file " << inputFile.absoluteFilePath().toAscii().constData() << "." << endl;
      result = false;
    }
  }

  if(result)
  {
    // read main header
    if(pPhilipsFile->readMainHeader(pPhilipsMainHeader) == false)
    {
      cout << "ERROR: When reading the mainheader or file is not an philips format image." << endl;
      result = false;
    }
  }

  if(result)
  {
    // check if it's an image file
    if(pPhilipsMainHeader->filtyp() != CPhilipsMainHeader::Image)
    {
      cout << "ERROR: Input file is not an image file" << endl;
      result = false;
    }
  }

  if(result)
  {
    D("Creating empty ecat image: '%s'", m_sOutputFileName.toAscii().data());
    pEcat7Image = new CECATFile(m_sOutputFileName, CECATMainHeader::ECAT7_Volume16);
    if(pEcat7Image->open(QIODevice::WriteOnly) == false)
    {
      cout << "ERROR: Could not write to outputfile - check permissions of directory or file!." << endl;
      result = false;
    }
  }

  if(result)
  {
    cout << "Converting: " << inputFile.absoluteFilePath().toAscii().constData() << endl;
    cout << "OutputFile: " << QFileInfo(m_sOutputFileName).absoluteFilePath().toAscii().constData() << endl;

    pEcat7ImageHeader = static_cast<CECAT7MainHeader*>(pEcat7Image->createEmptyMainHeader());

    if(pEcat7ImageHeader != NULL)
    {
      pEcat7ImageHeader->convertFrom(pPhilipsMainHeader);

      int numFrames = pPhilipsFile->numFrames();
      if(numFrames > NUMFRAMESLIMIT)
      {
        cout << "Warning: Can not create ECAT7 image file with more than "
             << NUMFRAMESLIMIT
             << " frames - will only create "
             << NUMFRAMESLIMIT
             << " frames" 
             << endl;

        numFrames = NUMFRAMESLIMIT;
      }

      int minSlice = pPhilipsFile->minSlice();

      for(int frame = 1; frame <= numFrames; ++frame)
      {
         cout << "Converting Frame: " << (frame) << " of " << numFrames << endl;

        // in philips format every slice has an own subheader, so
        // we need to hold one sub header of the philips file
        // to convert it to an ecat7subheader of the whole frame

        // take first subheader of the frame as the header of the whole frame
        CPhilipsSubHeader* pPhilipsSubHeader = NULL;
        bool bReadSubHeader = pPhilipsFile->readSubHeader(pPhilipsSubHeader, minSlice, frame);

        if(bReadSubHeader == false)
        {
          cout << "ERROR: When loading image subheader of slice " << minSlice
               << ", frame " << frame << endl;

          result = false;
          if(pPhilipsSubHeader != NULL)
            delete pPhilipsSubHeader;

          break;
        }

        char* pImageData;
        unsigned int len = 0;
        bool bReadMatrix = pPhilipsFile->readFrame(pImageData, len, frame);
        if(bReadMatrix == false)
        {
          cout << "ERROR: When loading image data of frame " << frame << endl;
          result = false;

          if(pPhilipsSubHeader != NULL)
            delete pPhilipsSubHeader;

          break;
        }

        short imgMaxValue = 0;
        short imgMinValue = SHRT_MAX;
        CECATSubHeader::Data_Type outputDataType = CECATSubHeader::UnknownDataType;
        CPhilipsSubHeaderImage* pPhilipsSubHeaderImage = static_cast<CPhilipsSubHeaderImage*>(pPhilipsSubHeader);

        switch(pPhilipsSubHeaderImage->datype())
        {
          case CPhilipsSubHeader::SignedShort:
          {
            D("found signed short source data");

            short xdim = pPhilipsSubHeaderImage->xdim();
            short ydim = pPhilipsSubHeaderImage->ydim();

            int numElements = len/sizeof(qint16);

            QByteArray buffer(pImageData, len);
            // cout << "len: " << len << endl;
            // cout << "num: " << numElements << endl;
            CDataArray<qint16> dataArray(&buffer, numElements);
            imgMaxValue = dataArray.maxValue();
            imgMinValue = dataArray.minValue();

            D("xdim    : %d", xdim);
            D("ydim    : %d", ydim);
            D("numSlices: %d", pPhilipsFile->numSlices());
            D("numElem : %d", numElements);
            D("imgMin: %d", imgMinValue);
            D("imgMax: %d", imgMaxValue);

            // cout << "imgMin " << imgMinValue << endl
            //      << "imgMax " << imgMaxValue << endl;


            outputDataType = CECATSubHeader::SunShort;

          }
          break;

          default:
          {
            cout << "ERROR: Could not handle datatype of philips image file." << endl;
            bUnknownDataType = true;
          }
        }

        if((bUnknownDataType == false) && (result != false))
        {
          CECAT7SubHeaderImage* pEcat7SubHeaderImage;
          pEcat7SubHeaderImage = static_cast<CECAT7SubHeaderImage*>(pEcat7Image->createEmptySubHeader());
          pEcat7SubHeaderImage->convertFrom(pPhilipsSubHeaderImage, pPhilipsMainHeader);
          pEcat7SubHeaderImage->setData_Type(outputDataType);

          D("imgMin: %d", imgMinValue);
          D("imgMax: %d", imgMaxValue);

          pEcat7SubHeaderImage->setImage_Min(imgMinValue);
          pEcat7SubHeaderImage->setImage_Max(imgMaxValue);

          bool ok;
          pEcat7SubHeaderImage->setScale_Factor(pPhilipsSubHeaderImage->scale_Factor(ok));

          if(ok == true)
          {
            pEcat7ImageHeader->setCalibration_Units(CECAT7MainHeader::Calibrated);
            pEcat7ImageHeader->setCalibration_Factor(1.0f);
            pEcat7ImageHeader->setData_Units("Bq/cc");
          }
          else
          {
            pEcat7ImageHeader->setCalibration_Units(CECAT7MainHeader::Uncalibrated);
            pEcat7ImageHeader->setData_Units("N/A");
          }

          // put in an annotation about being converted by mp2ecat
          pEcat7SubHeaderImage->setAnnotation("converted by " PACKAGE_NAME " " PACKAGE_VERSION);

          pEcat7Image->writeSubHeader(*pEcat7SubHeaderImage, frame);
          pEcat7Image->writeMatrix(pImageData, len, frame);
          delete pEcat7SubHeaderImage;
        }

        if(pPhilipsSubHeaderImage != NULL)
          delete pPhilipsSubHeaderImage;

        if(bUnknownDataType)
          break;
      }

      if(bUnknownDataType == false)
      {
        cout << "Writing main header and finalizing file" << endl;

        pEcat7Image->writeMainHeader(*pEcat7ImageHeader);
        result = true;
      }

      pEcat7Image->close();
      delete pEcat7ImageHeader;
    }
  }

  delete pEcat7Image;
  delete pPhilipsMainHeader;
  if(pPhilipsFile != NULL)
  {
    pPhilipsFile->close();
    delete pPhilipsFile;
  }

  RETURN(result);
  return result;
}

bool CApplication::convert2Img(const QFileInfo& inputFile)
{
  ENTER();
  bool result = true;
  bool bUnknownDataType = false;
  CECATFile* pEcat7Image = NULL;
  CECATMainHeader *pEcat7MainHeader = NULL;
  CECAT7MainHeader* pEcat7ImageHeader = NULL;
  CPhilipsFile* pPhilipsFile = NULL;
  CPhilipsMainHeader* pPhilipsMainHeader = NULL;

  // check if we can create the output file
  if(checkOutputFile(inputFile) == false)
    result = false;

  if(result)
  {
    pEcat7Image = new CECATFile(inputFile.filePath());
    if(pEcat7Image == NULL)
      result = false;
  }

  if(result)
  {
    if(pEcat7Image->open(QIODevice::ReadOnly) == false)
    {
      cout << "ERROR: Can't open file " << inputFile.absoluteFilePath().toAscii().constData() << "." << endl;
      result = false;
    }
  }

  if(result)
  {
    // read main header
    if(pEcat7Image->readMainHeader(pEcat7MainHeader) == false)
    {
      cout << "ERROR: When reading the mainheader or file is not an ECAT format image." << endl;
      result = false;
    }
    else
      pEcat7ImageHeader = static_cast<CECAT7MainHeader*>(pEcat7MainHeader);
  }

  if(result)
  {
    // check if it's an image file
    if(pEcat7Image->fileType() != CECATMainHeader::ECAT7_Volume16)
    {
      cout << "ERROR: Input file is not an image file" << endl;
      result = false;
    }
  }

  if(result)
  {
    D("Creating empty philips image: '%s'", m_sOutputFileName.toAscii().data());
    pPhilipsFile = new CPhilipsFile(m_sOutputFileName, CPhilipsMainHeader::Image);
    if(pPhilipsFile->open(QIODevice::WriteOnly) == false)
    {
      cout << "ERROR: Could not write to outputfile - check permissions of directory or file!." << endl;
      result = false;
    }
  }

  if(result)
  {
    cout << "Converting: " << inputFile.absoluteFilePath().toAscii().constData() << endl;
    cout << "OutputFile: " << QFileInfo(m_sOutputFileName).absoluteFilePath().toAscii().constData() << endl;

    pPhilipsMainHeader = pPhilipsFile->createEmptyMainHeader();
    if(pPhilipsMainHeader != NULL)
    {
      pPhilipsMainHeader->convertFrom(pEcat7ImageHeader);

      int numFrames = pEcat7Image->numFrames();
      if(numFrames > NUMFRAMESLIMIT)
      {
        cout << "Warning: Can not create Philips image file with more than "
             << NUMFRAMESLIMIT
             << " frames - will only create "
             << NUMFRAMESLIMIT
             << " frames" 
             << endl;

        numFrames = NUMFRAMESLIMIT;
      }

      int minSlice = 0;
      for(int frame = 1; frame <= numFrames; ++frame)
      {
         cout << "Converting Frame: " << (frame) << " of " << numFrames << endl;

        // in philips format every slice has an own subheader, so
        // we need to hold one sub header of the ecat file
        // to convert it to an philips subheader for every slice

        // take first subheader of the frame as the header of the whole frame
        CECATSubHeader* pECATSubHeader = NULL;
        CECAT7SubHeaderImage* pECATSubHeaderImage = NULL;
        bool bReadSubHeader = pEcat7Image->readSubHeader(pECATSubHeader, frame);
        if(bReadSubHeader == false)
        {
          cout << "ERROR: When loading image subheader of frame " << frame << endl;
          result = false;

          delete pECATSubHeaderImage;
          break;
        }
        else
          pECATSubHeaderImage = static_cast<CECAT7SubHeaderImage*>(pECATSubHeader);

        char* pImageData;
        unsigned int len = 0;
        bool bReadMatrix = pEcat7Image->readMatrix(pImageData, len, frame);
        if(bReadMatrix == false)
        {
          cout << "ERROR: When loading image data of frame " << frame << endl;
          result = false;

          delete pECATSubHeaderImage;
          break;
        }

        if(result != false)
        {
          short imgMaxValue = pECATSubHeaderImage->image_Max();
          short imgMinValue = pECATSubHeaderImage->image_Min();
          D("imgMin: %d", imgMinValue);
          D("imgMax: %d", imgMaxValue);

          // now convert the subheader
          CPhilipsSubHeaderImage* pPhilipsSubHeaderImage = static_cast<CPhilipsSubHeaderImage*>(pPhilipsFile->createEmptySubHeader());
          pPhilipsSubHeaderImage->convertFrom(pECATSubHeaderImage, pEcat7ImageHeader);

          // get dimensions of data
          short xDim = pECATSubHeaderImage->x_Dimension();
          short yDim = pECATSubHeaderImage->y_Dimension();
          short zDim = pECATSubHeaderImage->z_Dimension();

          int dataTypeSize;
          switch(pECATSubHeaderImage->data_Type())
          {
            case CECATSubHeader::UnknownDataType:
              W("unknown datatype found, assuming byte data");
              dataTypeSize = sizeof(char);
            break;

            case CECATSubHeader::ByteData:
              dataTypeSize = sizeof(char);
            break;
    
            case CECATSubHeader::VAX_Ix2:
            case CECATSubHeader::SunShort:
              dataTypeSize = sizeof(qint16);
            break;
      
            case CECATSubHeader::VAX_Ix4:
            case CECATSubHeader::SunLong:
              dataTypeSize = sizeof(qint32);
            break;

            case CECATSubHeader::IEEEFloat:
            case CECATSubHeader::VAX_Rx4:
              dataTypeSize = sizeof(float);
            break;
          }

          // in the philips file format each slice is saved separately in a matrix
          // in the ecat format (in volume mode), however, the whole volume is saved
          // as a 3D matrix instead. So we have to split the volume now into slices
          // and save them separately.
          char *p = pImageData;
          long matrixSize = xDim*yDim*dataTypeSize;
          for(int z = 1; z < zDim+1; z++)
          {
            // before writing the matrix to the file we need to calculate the image min/max
            // and the new scale factor
            QByteArray buffer = QByteArray::fromRawData(pImageData, xDim*yDim);
            CDataArray<qint16> dataArray(&buffer, xDim*yDim);
            short imgMinValue = dataArray.minValue();
            short imgMaxValue = dataArray.maxValue();
            
            pPhilipsSubHeaderImage->setImgmin(imgMinValue);
            pPhilipsSubHeaderImage->setImgmax(imgMaxValue);

            D("about to write slice %d", z);
            pPhilipsFile->writeMatrix(p, matrixSize, *pPhilipsSubHeaderImage, z, frame);
  
            // now advance p
            p += matrixSize;
          }

          //bool ok;
          //pEcat7SubHeaderImage->setScale_Factor(pPhilipsSubHeaderImage->scale_Factor(ok));
          //if(ok == true)
          //{
          //  pEcat7ImageHeader->setCalibration_Units(CECAT7MainHeader::Calibrated);
          //  pEcat7ImageHeader->setCalibration_Factor(1.0f);
          //  pEcat7ImageHeader->setData_Units("Bq/cc");
          //}
          //else
          //{
          //  pEcat7ImageHeader->setCalibration_Units(CECAT7MainHeader::Uncalibrated);
          //  pEcat7ImageHeader->setData_Units("N/A");
          //}

          //// put in an annotation about being converted by mp2ecat
          //pEcat7SubHeaderImage->setAnnotation("converted by " PACKAGE_NAME " " PACKAGE_VERSION);

          //pEcat7Image->writeSubHeader(*pEcat7SubHeaderImage, frame);
          //pEcat7Image->writeMatrix(pImageData, len, frame);
          //delete pEcat7SubHeaderImage;
        }

        delete pECATSubHeaderImage;
      }

      if(bUnknownDataType == false)
      {
        cout << "Writing main header and finalizing file" << endl;

        pPhilipsFile->writeMainHeader(*pPhilipsMainHeader);
        result = true;
      }

      pPhilipsFile->close();
      delete pPhilipsMainHeader;
    }
  }

  delete pEcat7ImageHeader;
  if(pEcat7Image != NULL)
  {
    pEcat7Image->close();
    delete pEcat7Image;
  }

  RETURN(result);
  return result;
}

bool CApplication::walkDirectory(const QDir& dir)
{
  ENTER();
  bool result = true;
  QString extension;

  if(m_bEcat2Img == false)
    extension = "img";
  else
    extension = "v";

  // this function will recursively walk through a directory and process all *.img files in it
  QFileInfoList dirEntries = dir.entryInfoList(QStringList("*." + extension), QDir::Files | QDir::NoSymLinks | QDir::AllDirs | QDir::NoDotAndDotDot);

  QFileInfo dirEntry;
  foreach(dirEntry, dirEntries)
  {
    if(dirEntry.isDir())
    {
      result = walkDirectory(QDir(dirEntry.absoluteFilePath()));
    }
    else
    {
      // check if we found a filename with a *.img suffix
      if(dirEntry.suffix().toLower() == extension)
      {
        if(m_bEcat2Img == false)
          convert2Ecat(dirEntry);
        else
          convert2Img(dirEntry);
      }
    }

    if(result == false)
      break;
  }

  RETURN(result);
  return result;
}
