#include "CApplication.h"
#include "CDataArray.h"

#include "config.h"

#include <CECATFile>
#include <CECAT7MainHeader>
#include <CECAT7SubHeaderImage>
#include <CPhilipsFile>
#include <CPhilipsMainHeader>
#include <CPhilipsExtendedMainHeader>
#include <CPhilipsSubHeaderImage>

#include <iostream>
#include <rtdebug.h>

#include <QString>
#include <QFileInfo>
#include <QMultiHash>
#include <QtCore/qmath.h>

#define NUMFRAMESLIMIT 511

using namespace std;

CApplication::CApplication()
 : m_bOverWrite(false),
   m_bPreserveDataType(false)
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

    if(args.contains("-n"))
    {
      m_sPatientName = args.value("-n");
      inputFileNames.removeAll(args.value("-n"));
    }

    if(args.contains("-p"))
    {
      m_bPreserveDataType = true;
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

  // if(bResult == false)
  //   showAppInfo();

  // if(args.contains("-v"))
  //   showVersion();
  // else if(bResult == false)
  //   showUsage(argc, argv);

  RETURN(bResult);
  return bResult;
}

bool CApplication::checkOutputFile(const QFileInfo& inputFile)
{
  ENTER();
  bool bResult = true;
  QFileInfo outputFile;

  if(m_sPreferedOutputFile.isEmpty() == true)
    outputFile = QFileInfo(inputFile.filePath() + ".v");
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

bool CApplication::process()
{
  ENTER();
  bool result = true;

  // // now we check out options and either convert bunch of files or a whole directory tree
  // if(m_bRecursive == false)
  // {
  QString fileName;
  foreach(fileName, m_sInputFileNames)
  {
    if(QFileInfo(fileName).exists() == true)
    {
      if(convertFile(QFileInfo(fileName)) == false)
      {
        result = false;
        break;
      }
    }
    else
    {
      cout << "ERROR: file " << fileName.toAscii().constData()
           << " doesn't exist. Skipping file." << endl;
    }
  }
  // }
  // else
  // {
  //   // lets walk the specified directory
  //   if(m_sInputFileNames.count() > 1)
  //   {
  //     cout << "ERROR: only one directory can be specified." << endl;
  //   }
  //   else
  //     result = walkDirectory(QDir(m_sInputFileNames[0]));
  // }

  RETURN(result);
  return result;
}

bool CApplication::convertFile(const QFileInfo& inputFile)
{
  ENTER();
  bool result = true;
  bool bUnknownDataType = false;
  CECATFile* pEcat7Image = NULL;
  CECAT7MainHeader* pEcat7ImageHeader = NULL;
  CPhilipsFile* pPhilipsFile = NULL;
  CPhilipsMainHeader* pPhilipsMainHeader = NULL;
  CPhilipsExtendedMainHeader* pPhilipsExtendedMainHeader = NULL;

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
    // read the extended main header
    if(pPhilipsFile->readExtendedMainHeader(pPhilipsExtendedMainHeader) == false)
    {
      cout << "No extended main header found." << endl;
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
    pEcat7ImageHeader->convertFrom(pPhilipsMainHeader, pPhilipsExtendedMainHeader);

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
    int maxSlice = pPhilipsFile->maxSlice();
    short sliceThickness = pPhilipsMainHeader->slcthk();
    for(int frame = 0; frame < numFrames; ++frame)
    {
      QByteArray pImageData;
      cout << "Converting Frame: " << (frame+1) << " of " << numFrames << endl;

      // in philips format every slice has an own subheader, so
      // we need to hold one sub header of the philips file
      // to convert it to an ecat7subheader of the whole frame
      CPhilipsSubHeaderImage* pLastPhilipsSubHeaderImage = NULL;
      CECATSubHeader::Data_Type outputDataType = CECATSubHeader::UnknownDataType;

      short imgMaxValue = 0;
      short imgMinValue = SHRT_MAX;
      for(int slice = minSlice; slice <= maxSlice; slice += sliceThickness)
      {
        cout << "Converting Slice: " << (slice) << endl;

        CPhilipsSubHeader* pPhilipsSubHeader = NULL;
        bool bReadSubHeader = pPhilipsFile->readSubHeader(pPhilipsSubHeader, slice, frame+1);
        if(bReadSubHeader == false)
        {
          cout << "ERROR: When loading image subheader of slice " << (slice) << endl;
          result = false;
          delete pPhilipsSubHeader;
          break;
        }

        CPhilipsSubHeaderImage* pPhilipsSubHeaderImage = static_cast<CPhilipsSubHeaderImage*>(pPhilipsSubHeader);

        QByteArray* buffer;
        bool bReadMatrix = pPhilipsFile->readMatrix(buffer, slice, frame+1);
        if(bReadMatrix == false)
        {
          cout << "ERROR: When loading image data of  slice " << (slice) << endl;
          result = false;
          delete buffer;
          break;
        }

        char* pDestData = NULL;

        short sliceMaxValue = 0;
        short sliceMinValue = SHRT_MAX;

        switch(pPhilipsSubHeaderImage->datype())
        {
          case CPhilipsSubHeader::SignedShort:
          case CPhilipsSubHeader::UnsignedShort:
          {
            D("found short source data");
            STARTCLOCK("converting data");
            short xdim = pPhilipsSubHeaderImage->xdim();
            short ydim = pPhilipsSubHeaderImage->ydim();

            int numElements =  (xdim * ydim) / sizeof(qint16);
            CDataArray<qint16> dataArray(buffer, numElements);
            sliceMaxValue = dataArray.maxValue();
            sliceMinValue = dataArray.minValue();
            // sliceMaxValue = pPhilipsSubHeaderImage->imgmax();
            // sliceMinValue = pPhilipsSubHeaderImage->imgmin();

            D("sliceMin: %d", sliceMinValue);
            D("sliceMax: %d", sliceMaxValue);

            if(fabs(sliceMaxValue) > fabs(imgMaxValue))
              imgMaxValue = sliceMaxValue;

            if(fabs(sliceMinValue) < fabs(imgMinValue))
              imgMinValue = sliceMinValue;

            STOPCLOCK("converting data");

            outputDataType = CECATSubHeader::SunShort;

           }
           break;

          default:
          {
            cout << "ERROR: Could not handle datatype of philips image file." << endl;
            bUnknownDataType = true;
          }
        }

        if(bUnknownDataType == false)
        {
          pImageData.append(*buffer);
        }

        delete buffer;

        // in philips format every slice has its own subheader, so
        // we need to hold one sub header of the philips file (the last one)
        // to convert it to an ecat7subheader of the whole frame
        if(slice != maxSlice)
          delete pPhilipsSubHeaderImage;
        else
          pLastPhilipsSubHeaderImage = pPhilipsSubHeaderImage;

        if(bUnknownDataType)
          break;
      }

      if(bUnknownDataType == false)
      {
        CECAT7SubHeaderImage* pEcat7SubHeaderImage;
        pEcat7SubHeaderImage = static_cast<CECAT7SubHeaderImage*>(pEcat7Image->createEmptySubHeader());
        pEcat7SubHeaderImage->convertFrom(pLastPhilipsSubHeaderImage, pPhilipsMainHeader);
        pEcat7SubHeaderImage->setData_Type(outputDataType);

        D("imgMin: %d", imgMinValue);
        D("imgMax: %d", imgMaxValue);

        pEcat7SubHeaderImage->setImage_Min(imgMinValue);
        pEcat7SubHeaderImage->setImage_Max(imgMaxValue);

        D("suvScale     : %f", pLastPhilipsSubHeaderImage->suvscl());

        if(pLastPhilipsSubHeaderImage->suvscl() > 0.0)
        {
          pEcat7ImageHeader->setCalibration_Units(CECAT7MainHeader::Calibrated);
          pEcat7ImageHeader->setCalibration_Factor(1.0f);
          pEcat7ImageHeader->setData_Units("BQML");

          // now we calculate the new scale factor
          float suvScale = pLastPhilipsSubHeaderImage->suvscl();
          float dosage = pPhilipsMainHeader->activity() * 1000000.0f; // Bq
          float deltaT = static_cast<float>(pPhilipsMainHeader->injtim()); // s
          float halfLife = pPhilipsMainHeader->halfLife() * 60.0f; // s
          float patientWeight = static_cast<float>(pPhilipsMainHeader->weight()); // g

          float factor = suvScale * dosage * qExp(-qLn(2) * (deltaT/halfLife)) / patientWeight;

          D("dosage       : %f", dosage);
          D("deltaT       : %f", deltaT);
          D("halfLife     : %f", halfLife);
          D("patientWeight: %f", patientWeight);
          D("factor       : %f",factor);

          pEcat7SubHeaderImage->setScale_Factor(factor);
        }
        else
        {
          pEcat7SubHeaderImage->setScale_Factor(pLastPhilipsSubHeaderImage->imgscl());
        }

        // put in an annotation about being converted by mp2ecat
        pEcat7SubHeaderImage->setAnnotation("converted by " PACKAGE_NAME " " PACKAGE_VERSION);

        pEcat7Image->writeSubHeader(*pEcat7SubHeaderImage, frame+1);
        pEcat7Image->writeMatrix(pImageData, frame+1);
        delete pEcat7SubHeaderImage;
      }

      delete pLastPhilipsSubHeaderImage;

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

  delete pEcat7Image;
  delete pPhilipsMainHeader;
  if(pPhilipsFile)
    pPhilipsFile->close();
  delete pPhilipsFile;

  RETURN(result);
  return result;
}
