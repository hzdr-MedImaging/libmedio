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
   m_bConvertToShort(false)
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
      bResult = checkOutputFile(args.value("-o"));
      inputFileNames.removeAll(args.value("-o"));
    }

    if(args.contains("-n"))
    {
      m_sPatientName = args.value("-n");
      inputFileNames.removeAll(args.value("-n"));
    }

    if(args.contains("-c"))
    {
      m_bConvertToShort = true;
    }

    // we check if there is one and only one input file available
    // we do not support processing of multiple concorde micropet image files
    if(inputFileNames.isEmpty())
    {
      bResult = false;
      cout << "ERROR: no concorde micropet image file provided" << endl;
    }
    else if(inputFileNames.count() > 1)
    {
      bResult = false;
      cout << "ERROR: multiple concorde micropet image files found - not supported" << endl;
    }
    else
    {
      // check if concorde micropet image file is available
      if(!inputFileNames.first().isEmpty())
      {
        QFileInfo fileInfo(inputFileNames.first());
        if(fileInfo.exists() && fileInfo.isFile() && fileInfo.isReadable())
        {
          fileInfo.makeAbsolute();
          m_sInputFileName = fileInfo.filePath();
        }
        else
        {
          bResult = false;
          cout << "ERROR: image file \"" << inputFileNames.first().toAscii().data() << "\" is not existing|a file|readable" << endl;
        }
      }
      else
      {
        bResult = false;
        cout << "ERROR: no concorde micropet image file provided" << endl;
      }
    }

    if(bResult && args.contains("-o") == false)
      bResult = checkOutputFile(m_sInputFileName + QString(".v"));
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

bool CApplication::checkOutputFile(QString sFileName)
{
  ENTER();
  bool bResult = true;
  QFileInfo fileInfo(sFileName);
  if(!m_bOverWrite)
  {
    if(fileInfo.exists())
    {
      bResult = false;
      cout << "ERROR: output file already exists" << endl;
    }
  }
  else
  {
    if(fileInfo.exists() && !fileInfo.isFile() && !fileInfo.isWritable())
    {
      bResult = false;
      cout << "ERROR: can not overwrite non-file" << endl;
    }
  }

  if(bResult)
  {
    // check if input file equals output file
    fileInfo.makeAbsolute();
    m_sOutputFileName = fileInfo.filePath();
    if(m_sOutputFileName == m_sInputFileName)
    {
      bResult = false;
      cout << "ERROR: will not overwrite input file" << endl;
    }
  }

  if(bResult)
  {
    bResult = checkOutputDir(fileInfo.dir().canonicalPath());
  }

  RETURN(bResult);
  return bResult;
}

bool CApplication::checkOutputDir(QString sDirectory)
{
  ENTER();
  bool bResult = true;
  QFileInfo destinationDir(sDirectory);
  if(!destinationDir.isWritable())
  {
    cout << "ERROR: Could not write to outputfile - check permissions of directory or file!." << endl;
    bResult = false;
  }
  RETURN(bResult);
  return bResult;
}

void CApplication::showUsage(int, char* argv[])
{
  ENTER();
  // output usage information on the console.
  cout << "Usage: " << argv[0] << " <options> <file.img>" << endl
       << "Options:" << endl
       << "  -o <file>    : ECAT image (*.v) to which microPET image is converted" << endl
       << "  -n <string>  : override patient name in ECAT output file" << endl
       << "  -c           : convert float mpet data to short integer values in ECAT" << endl
       << "                 (this implies a slight precision loss)" << endl
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
       << "(" __DATE__ ") Copyright (c) 2006-2011 by Hagen Moelle, Jens Langner / www.hzdr.de" << endl << endl;
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

bool CApplication::convertFile()
{
  ENTER();
  bool bResult = true;
  bool bUnknownDataType = false;
  CMedIOData* pSrcImageVolume = NULL;
  CConcordeMainHeader* pSrcImageHeader = NULL;
  CECATFile* ecat7Image = NULL;
  CECAT7MainHeader* pEcat7ImgHeader = NULL;

  //create new CMedIOData object from file
  pSrcImageVolume = CMedIODataFactory::create(m_sInputFileName);
  if(pSrcImageVolume == NULL)
  {
    cout << "ERROR: Image source file format is unknown." << endl;
    bResult = false;
  }

  if(bResult)
  {
    bool bOpenSource = pSrcImageVolume->open(QIODevice::ReadOnly);
    if(!bOpenSource || !(pSrcImageVolume->rtti() == CMedIOData::ConcordeMicropet))
    {
      cout << "ERROR: When opening file or file is not from concorde microPET." << endl;
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
    D("Creating empty ecat image: %s", m_sOutputFileName.toAscii().data());
    ecat7Image = new CECATFile(m_sOutputFileName, CECATMainHeader::ECAT7_Volume16);
    if(!ecat7Image->open(QIODevice::WriteOnly))
    {
      cout << "ERROR: Could not write to outputfile - check permissions of directory or file!." << endl;
      bResult = false;
    }
  }

  if(bResult)
  {
    pEcat7ImgHeader = static_cast<CECAT7MainHeader*>(ecat7Image->createEmptyMainHeader());
    
    pEcat7ImgHeader->convertFrom(pSrcImageHeader);
    pEcat7ImgHeader->setPatient_Name(m_sPatientName.toAscii().data());
    
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
      bool bReadSubHeader = ((CConcordeFile*)pSrcImageVolume)->readSubHeader(pSrcImgSubHeader, i+1);
      bool bReadMatrix = ((CConcordeFile*)pSrcImageVolume)->readMatrix(pImgData, i+1);
      if(!bReadSubHeader || !bReadMatrix)
      {
        cout << "ERROR: When loading subheader or reading data." << endl;
        bResult = false;
        delete pImgData;
        delete pSrcImgSubHeader;

        break;
      }
      
      cout << "Converting Frame: " << (i+1) << " of " << iNrFrames;
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

          if(m_bConvertToShort)
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

          if(m_bConvertToShort)
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

  return bResult;
}
