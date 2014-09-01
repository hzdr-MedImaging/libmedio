#include "CApplication.h"

#include "config.h"
#include "CDataArray.h"

#include <CECATFile>
#include <CECAT7MainHeader>
#include <CECAT7SubHeaderImage>
#include <CConcordeMainHeader>
#include <CConcordeFrameHeader>
#include <CConcordeImage>
#include <CMedIOData>
#include <CMedIODataFactory>
#include <CMedIO>

#include <math.h>
#include <limits.h>
#include <iostream>
#include <rtdebug.h>

#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QMultiHash>

#define NUMFRAMESLIMIT 511

using namespace std;

CApplication::CApplication()
 : m_bOverWrite(false),
   m_bPreserveDataType(false),
   m_bRecursive(false)
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

    if(args.contains("-r"))
    {
      m_bRecursive = true;
    }

    // we check if there is one and only one input file available
    // we do not support processing of multiple concorde micropet image files
    if(inputFileNames.isEmpty())
    {
      bResult = false;
      cout << "ERROR: no concorde micropet image file provided" << endl;
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

void CApplication::showUsage(int, char* argv[])
{
  ENTER();
  // output usage information on the console.
  cout << "Usage: " << argv[0] << " <options> <file.img|directory>" << endl
       << "Options:" << endl
       << "  -o <file>    : ECAT image (*.v) to which microPET image is converted" << endl
       << "  -n <string>  : override patient name in ECAT output file" << endl
       << "  -p           : preserve data type as is and do not convert to short values" << endl
       << "                 (this will give you the highest precision on cost of space)" << endl
       << "  -r           : walk the specified directory recursively and convert all *.img" << endl
       << "                 file in it in one run." << endl
       << "  -f           : force overwriting of existing files" << endl;
  LEAVE();
  return;
}

void CApplication::showAppInfo()
{
  ENTER();
  // output some general program information
  cout << endl
       << PACKAGE_STRING << " - converts microPET image file to ECAT file" << endl
       << "(" __DATE__ ") Copyright (c) 2006-2014 Jens Maus, Hagen Moelle / www.hzdr.de" << endl << endl;
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
                  << "  Copyright (c) 2006-2007 Trolltech Inc." << endl << endl

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
  bool result = false;

  // now we check out options and either convert bunch of files or a whole directory tree
  if(m_bRecursive == false)
  {
    QString fileName;
    foreach(fileName, m_sInputFileNames)
    {
      if(convertFile(QFileInfo(fileName)) == false)
      {
        result = false;
        break;
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

bool CApplication::convertFile(const QFileInfo& inputFile)
{
  ENTER();
  bool bResult = true;
  bool bUnknownDataType = false;
  CMedIOData* pSrcImageVolume = NULL;
  CConcordeMainHeader* pSrcImageHeader = NULL;
  CECATFile* ecat7Image = NULL;
  CECAT7MainHeader* pEcat7ImgHeader = NULL;

  // check if we can create the output file
  if(checkOutputFile(inputFile) == false)
    bResult = false;

  //create new CMedIOData object from file
  if(bResult)
  {
    pSrcImageVolume = CMedIODataFactory::create(inputFile.filePath());
    if(pSrcImageVolume == NULL)
    {
      cout << "ERROR: Unknown image file format (" << inputFile.filePath().toAscii().constData() << ")." << endl;
      bResult = false;
    }
  }

  if(bResult)
  {
    bool bOpenSource = pSrcImageVolume->open(QIODevice::ReadOnly);
    if(!bOpenSource || !(pSrcImageVolume->dataFormat() == CMedIOData::ConcordeMicropet))
    {
      cout << "ERROR: Couldn't open file or file not a concorde microPET file." << endl;
      bResult = false;
    }
  }

  if(bResult)
  {
    bool bReadMainHeader = ((CConcordeFile*)pSrcImageVolume)->readMainHeader(pSrcImageHeader);
    if(!bReadMainHeader ||
        !(((CConcordeFile*)pSrcImageVolume)->fileType() == CConcordeFile::ConcordeMicropet_Image))
    {
      cout << "ERROR: When reading the mainheader or file is not an concorde microPET image." << endl;
      bResult = false;
    }
  }

  if(bResult)
  {
    if(pSrcImageHeader->dataType() == CConcordeMainHeader::UnknownDataType)
    {
      cout << "ERROR: Unknown datatype in concorde microPET image." << endl;
      bResult = false;
    }
  }

  if(bResult)
  {
    D("Creating empty ecat image: '%s'", m_sOutputFileName.toAscii().data());
    ecat7Image = new CECATFile(m_sOutputFileName, CECATMainHeader::ECAT7_Volume16);
    if(!ecat7Image->open(QIODevice::WriteOnly))
    {
      cout << "ERROR: Could not write to outputfile - check permissions of directory or file!." << endl;
      bResult = false;
    }
  }

  if(bResult)
  {
    cout << "Converting: " << inputFile.absoluteFilePath().toAscii().constData() << endl;
    cout << "OutputFile: " << QFileInfo(m_sOutputFileName).absoluteFilePath().toAscii().constData() << endl;

    pEcat7ImgHeader = static_cast<CECAT7MainHeader*>(ecat7Image->createEmptyMainHeader());
    pEcat7ImgHeader->convertFrom(pSrcImageHeader);

    if(m_sPatientName.isEmpty() == false)
      pEcat7ImgHeader->setPatient_Name(m_sPatientName.toAscii().constData());

    if(QString(pEcat7ImgHeader->study_Description()).trimmed().isEmpty())
      pEcat7ImgHeader->setStudy_Description(inputFile.fileName().toAscii().constData());
    
    int iNrFrames = pSrcImageHeader->totalFrames();
    if(iNrFrames > NUMFRAMESLIMIT)
    {
      cout << "Warning: Can not create ECAT7 image file with more than "
           << NUMFRAMESLIMIT
           << " frames - will only create "
           << NUMFRAMESLIMIT
           << " frames" 
           << endl;

      iNrFrames = NUMFRAMESLIMIT;
    }

    unsigned int iFramesize = pSrcImageHeader->frameSize();
    for(int i = 0; i < iNrFrames; i++)
    {
      QByteArray* pImgData = NULL;
      CConcordeFrameHeader* pSrcImgSubHeader = NULL;

      cout << "Converting Frame: " << (i+1) << " of " << iNrFrames;

      bool bReadSubHeader = ((CConcordeFile*)pSrcImageVolume)->readSubHeader(pSrcImgSubHeader, i+1);
      if(!bReadSubHeader)
      {
        cout << "ERROR: When loading image subheader of frame " << i+1 << endl;
        bResult = false;
        delete pSrcImgSubHeader;
        break;
      }

      bool bReadMatrix = ((CConcordeFile*)pSrcImageVolume)->readMatrix(pImgData, i+1);
      if(!bReadMatrix)
      {
        cout << "ERROR: When loading image data of frame " << i+1 << endl;
        bResult = false;
        delete pImgData;
        break;
      }
      
      char* pDestData = NULL;
      float imgMaxValue = 0.0F;
      float imgMinValue = 10000.0F;
      float imgScaleFactor = 0.0F;
      CECATSubHeader::Data_Type outputDataType = CECATSubHeader::UnknownDataType;

      switch(pSrcImageHeader->dataType())
      {
        case CConcordeMainHeader::IntelShort:
        case CConcordeMainHeader::SunShort:
        {
          D("found short source data");

          cout << " (short->short)" << endl;

          STARTCLOCK("converting data");
          int iNumElements = iFramesize/sizeof(qint16);
          CDataArray<qint32> dataArray(pImgData, iNumElements);
          imgMaxValue = dataArray.maxValue();
          imgMinValue = dataArray.minValue();

          // as we don't have to rescale any data we simply use the same scale factor
          // and thus we don't need to calculate an own one here or rescale any data
          imgScaleFactor = 1.0f;

          STOPCLOCK("converting data");

          outputDataType = CECATSubHeader::SunShort;
        }
        break;

        case CConcordeMainHeader::IntelInt:
        case CConcordeMainHeader::SunInt:
        {
          D("found integer source data");

          STARTCLOCK("converting data");

          if(m_bPreserveDataType == false)
          {
            cout << " (long->short)" << endl;

            int iNumElements = iFramesize/sizeof(qint32);
            CDataArray<qint32> dataArray(pImgData, iNumElements);
            pDestData = new char[iNumElements*sizeof(qint16)];
            imgMaxValue = dataArray.maxValue();
            imgMinValue = dataArray.minValue();
            imgScaleFactor = dataArray.maxDistance()/float(SHRT_MAX);
            qint16* pDestDataElement = (qint16*)pDestData;

            for(int i = 0; i < iNumElements; i++, pDestDataElement++)
              *pDestDataElement = qRound(1.0f/imgScaleFactor*dataArray[i]);

            D("Scalefactor: %f, Scale factor in header: %f", imgScaleFactor, pSrcImgSubHeader->scaleFactor());
            delete pImgData;
            pImgData = new QByteArray(pDestData,iNumElements*sizeof(short));
            delete [] pDestData;

            outputDataType = CECATSubHeader::SunShort;
          }
          else
          {
            cout << " (long->long)" << endl;

            // if we keep the same precision we don't have to convert something at all,
            // we just have to set our own rescaling factor to 1 and the min/max values
            // right
            imgScaleFactor = 1.0f;
            imgMinValue = -32767;
            imgMaxValue = 32767;

            outputDataType = CECATSubHeader::SunLong;
          }

          STOPCLOCK("converting data");
        }
        break;

        case CConcordeMainHeader::IntelFloat:
        case CConcordeMainHeader::IEEEFloat:
        {
          D("found float source data");

          STARTCLOCK("converting data");

          if(m_bPreserveDataType == false)
          {
            cout << " (float->short)" << endl;

            int iNumElements = iFramesize/sizeof(float);
            CDataArray<float> dataArray(pImgData, iNumElements);
            pDestData = new char[iNumElements*sizeof(qint16)];
            imgMaxValue = dataArray.maxValue();
            imgMinValue = dataArray.minValue();
            imgScaleFactor = dataArray.maxDistance()/float(SHRT_MAX);
            qint16* pDestDataElement = (qint16*)pDestData;

            for(int i = 0; i < iNumElements; i++, pDestDataElement++)
              *pDestDataElement = qRound(1.0f/imgScaleFactor*dataArray[i]);

            D("Scalefactor: %f, Scale factor in header: %f", imgScaleFactor, pSrcImgSubHeader->scaleFactor());
            delete pImgData;
            pImgData = new QByteArray(pDestData,iNumElements*sizeof(qint16));
            delete [] pDestData;

            outputDataType = CECATSubHeader::SunShort;
          }
          else
          {
            cout << " (float->float)" << endl;

            // if we keep the same precision we don't have to convert something at all,
            // we simply have to modify the scale factor to include the branching fraction
            imgScaleFactor = 1.0f;
            imgMinValue = -32767;
            imgMaxValue = 32767;

            outputDataType = CECATSubHeader::IEEEFloat;
          } 

          STOPCLOCK("converting data");
        }
        break;

        default:
        {
          cout << "ERROR: Could not handle datatype of micropet image." << endl;
          bUnknownDataType = true;
        }
        break;
      }

      if(!bUnknownDataType)
      {
        CECAT7SubHeaderImage* pEcat7ImgSubHeader;
        pEcat7ImgSubHeader = static_cast<CECAT7SubHeaderImage*>(ecat7Image->createEmptySubHeader());
        pEcat7ImgSubHeader->convertFrom(pSrcImgSubHeader, pSrcImageHeader);
        pEcat7ImgSubHeader->setData_Type(outputDataType);

        // we have to recalculate the scale factor for the ECAT format. Important to note is that within
        // the concorde format the isotope branching fraction is NOT included contrary to what is done
        // in the ECAT format. Thus, we have to calculate the ECAT scale factor as followed:
        //
        // S = mS * iS / bf
        //
        // where S denotes the new ECAT scale factor, mS the original scale factor of the mpet file,
        // iS the integer scale factor we used to scale the float image to integer/short values and bf
        // the isotope branching fraction.
        pEcat7ImgSubHeader->setScale_Factor(pSrcImgSubHeader->scaleFactor()*imgScaleFactor/pSrcImageHeader->isotopeBranchingFraction());

        D("imgMax: %g (%g,%g), imgMin: %g (%g,%g)", imgMaxValue, imgMaxValue*imgScaleFactor, 1.0f/imgScaleFactor*imgMaxValue, imgMinValue, imgMinValue*imgScaleFactor, 1.0f/imgScaleFactor*imgMinValue);

        if(fabs(imgMaxValue) > fabs(imgMinValue))
        {
          pEcat7ImgSubHeader->setImage_Min(qRound(1.0f/imgScaleFactor*imgMinValue));
          pEcat7ImgSubHeader->setImage_Max(SHRT_MAX);
        }
        else
        {
          pEcat7ImgSubHeader->setImage_Max(qRound(1.0f/imgScaleFactor*imgMaxValue));
          pEcat7ImgSubHeader->setImage_Min(SHRT_MIN);
        }

        // put in an annotation about being converted by mp2ecat
        pEcat7ImgSubHeader->setAnnotation("converted by " PACKAGE_NAME " " PACKAGE_VERSION);

        ecat7Image->writeSubHeader(*pEcat7ImgSubHeader, i+1);
        ecat7Image->writeMatrix(*pImgData,i+1);
        delete pEcat7ImgSubHeader;
      }
      delete pImgData;
      delete pSrcImgSubHeader;

      if(bUnknownDataType)
        break;
    }

    if(!bUnknownDataType)
    {
      cout << "Writing main header and finalizing file" << endl;
      ecat7Image->writeMainHeader(*pEcat7ImgHeader);
      bResult = true;
    }
    ecat7Image->close();
    delete pEcat7ImgHeader;
  }

  delete ecat7Image;
  delete pSrcImageHeader;
  if(pSrcImageVolume)
    pSrcImageVolume->close();
  delete pSrcImageVolume;

  RETURN(bResult);
  return bResult;
}

bool CApplication::walkDirectory(const QDir& dir)
{
  ENTER();
  bool result = true;

  // this function will recursively walk through a directory and process all *.img files in it
  QFileInfoList dirEntries = dir.entryInfoList(QStringList("*.img"), QDir::Files | QDir::NoSymLinks | QDir::AllDirs | QDir::NoDotAndDotDot);

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
      if(dirEntry.suffix().toLower() == "img")
        convertFile(dirEntry);
    }

    if(result == false)
      break;
  }

  RETURN(result);
  return result;
}
