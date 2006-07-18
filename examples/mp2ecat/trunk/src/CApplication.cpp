//! @file CCmdLineStart.cpp
//! @brief contains the implementation of the class CCmdLineStart
//! @author Hagen Moelle
//! @date 11/13/2004

#include "CApplication.h"

#include <CECATFile>
#include <CECAT7MainHeader>
#include <CECAT7SubHeaderImage>
#include <CConcordeMainHeader>
#include <CConcordeFrameHeader>
#include <CConcordeImage>
#include <CMedIOData>
#include <CMedIODataFactory>

#include <math.h>
#include <iostream>
#include <rtdebug.h>

#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QMultiHash>

using namespace std;

CApplication::CApplication()
{
	ENTER();
	LEAVE();
}

CApplication::~CApplication()
{
	ENTER();
	LEAVE();
}

bool CApplication::parseCmdLine(int argc, char* argv[])
{
	ENTER();
	bool bResult = false;
	// put all arguments in a temporary MultiHash
	QMultiHash<QString, QString> args;

	// if the user has specified some commandline options
	// lets process and parse them.
	for(int i=1; i < argc; i++)
	{
		QString option(argv[i]);

		if(option[0] == '-')
		{
			if(i+1 < argc && argv[i+1][0] != '-')
			{
				args.insert(option, argv[i+1]);
			}
			else
				args.insert(option, "");
		}
		else
			args.insert("infile", argv[i]);
	}

	// now we check/process the different options according to their
	// priority
	if(args.contains("-i") == false ||
		 args.contains("-o") == false)
	{
		bResult = false;
		cout << endl << "Error: input or output file not found" << endl << endl;;
	}
	else if(args.contains("-h") == false &&
		 args.contains("-v") == false)
	{
		bResult = true;
		if(args.contains("-i"))
		{
			m_sInputFileName = args.value("-i");
		}
		if(args.contains("-o"))
		{
			m_sOutputFileName = args.value("-o");
		}
		if(args.contains("-n"))
		{
			m_sPatientName = args.value("-n");
		}
	}

	if(args.contains("-v"))
	{
		//cout << "Using:" << endl
		//		 << "  libmedio " << CMedIO::version().toAscii().constData() <<  " ("
		//		 << CMedIO::buildDate().toAscii().constData() << ")" << endl;
		//		 //<< "  " << CMedIO::copyright().toAscii().constData() << endl;
	}
	else if(bResult == false) // output usage information on the console.
	{
		cout << "Usage: " << argv[0] << " <options>" << endl
				 << "Options:" << endl
				 << "  -i <file>    : microPET file (*.img) which should be converted to ECAT format" << endl
				 << "  -o <file>    : ECAT image file (*.v) to which microPET file is converted" << endl
				 << "  -n <string>  : name of patient in ECAT file" << endl;
	}
	RETURN(bResult);
	return bResult;
}

bool CApplication::convertFile()
{
	bool bResult = false;
	bool bUnknownDataType = false;

	QFileInfo absInputPath(m_sInputFileName);
	absInputPath.makeAbsolute();
	QString sAbsInputPath(absInputPath.filePath().toAscii().data());
	QFileInfo absOutputPath(m_sOutputFileName);
	absOutputPath.makeAbsolute();
	QString sAbsOutputPath(absOutputPath.filePath().toAscii().data());
	QFileInfo destinationDir(absOutputPath.dir().canonicalPath());

	cout << "Opening image file: " << sAbsInputPath.toAscii().data() << endl;
	cout << "Writing to file: " << sAbsOutputPath.toAscii().data() << endl;
	
	if(!destinationDir.isWritable())
	{
		cout << "Error: Could not write to outputfile - check permissions of directory or file!." << endl;
		bResult = false;
	}
	else
	{
		//create new CMedIOData object from file
		CMedIOData* pSrcImageVolume = CMedIODataFactory::create(sAbsInputPath);
		if(pSrcImageVolume == NULL)
		{
			cout << "Error: Image source file format is unknown." << endl;
			bResult = false;
		}
		else
		{
			if(!pSrcImageVolume->open(QIODevice::ReadOnly) || !(pSrcImageVolume->rtti() == CMedIOData::ConcordeMicropet))
			{
				cout << "Error: When opening file or file is not from concorde microPET." << endl;
				bResult = false;
			}
			else
			{
				CConcordeMainHeader* pSrcImageHeader = NULL;
				if(!((CConcordeFile*)pSrcImageVolume)->readMainHeader(pSrcImageHeader) ||
						!(((CConcordeFile*)pSrcImageVolume)->fileType() == CConcordeFile::ConcordeMicropet_Image))
				{
					cout << "Error: When reading the mainheader or file is not an concorde microPET image." << endl;
					bResult = false;
				}
				else
				{
					D("Creating empty ecat image: %s", sAbsOutputPath.toAscii().data());
					CECATFile ecat7Image(sAbsOutputPath, CECATMainHeader::ECAT7_Volume16);
					if(!ecat7Image.open(QIODevice::WriteOnly))	
					{
						cout << "Error: Could not write to outputfile - check permissions of directory or file!." << endl;
						bResult = false;
					}
					else
					{
						CECAT7MainHeader* pEcat7ImgHeader = static_cast<CECAT7MainHeader*>(ecat7Image.createEmptyMainHeader());
						
						pEcat7ImgHeader->convertFrom(pSrcImageHeader);
						pEcat7ImgHeader->setPatient_Name(m_sPatientName.toAscii().data());
						
						int iNrFrames = pSrcImageHeader->totalFrames();
						if(iNrFrames > 255)
						{
							cout << "Warning: Can not create ECAT7 image file with more than 255 frames - will only create 255 frames" << endl;
							iNrFrames = 255;
						}

						unsigned int iFramesize = pSrcImageHeader->frameSize();
						for(int i = 0; i < iNrFrames; i++)
						{
							QByteArray* pImgData = NULL;
							CConcordeFrameHeader* pSrcImgSubHeader = NULL;
							if(!((CConcordeFile*)pSrcImageVolume)->readSubHeader(pSrcImgSubHeader, i+1) || !((CConcordeFile*)pSrcImageVolume)->readMatrix(pImgData, i+1))
							{
								cout << "Error: When loading subheader or reading data." << endl;
								bResult = false;
								if(pImgData)
									delete pImgData;
								if(pSrcImgSubHeader)
									delete pSrcImgSubHeader;
								break;
							}
							else
							{	
								cout << "Converting Frame: " << (i+1) << " of " << iNrFrames << endl;
								char* pDestData = NULL;
								float imgMaxValue = 0.0F;
								float imgMinValue = 10000.0F;
								float imgRange = 0.0F;
								float imgScaleFactor = 0.0F;

								switch(pSrcImageHeader->dataType())
								{
									case CConcordeMainHeader::IntelShort:
									case CConcordeMainHeader::SunShort:
									{
										short* pData = (short*)pImgData->data();
										pDestData = new char[iFramesize];
										for(unsigned int j = 0; j < iFramesize/2; j++, pData++)
										{
											if(imgMaxValue < *pData)
												imgMaxValue = *pData;
											if(imgMinValue > *pData)
												imgMinValue = *pData;
										}
										if(fabs(imgMaxValue) > fabs(imgMinValue))
										{
											imgRange = fabs(imgMaxValue);
										}
										else
										{
											imgRange = fabs(imgMinValue);
										}
										imgScaleFactor = imgRange/32767.0;
										pData = (short*)pImgData->data();
										short* pShortDestData = (short*)pDestData;
										for(unsigned int j = 0; j < iFramesize/2; j++, pData++,pShortDestData++)
										{
											*pShortDestData = (short)floor(*pData*32767.0/imgRange);
											
										}
										D("Scalefactor: %f", imgScaleFactor);
										D("Scale factor in header: %f", pSrcImgSubHeader->scaleFactor());
										cout << "Image min: " << imgMinValue << ", image max: " << imgMaxValue << endl;
										delete pImgData;
										pImgData = new QByteArray(pDestData,iFramesize);
										delete [] pDestData;
									}
									break;
									case CConcordeMainHeader::IntelInt:
									case CConcordeMainHeader::SunInt:
									{
										int* pData = (int*)pImgData->data();
										pDestData = new char[iFramesize/2];
										for(unsigned int j = 0; j < iFramesize/4; j++, pData++)
										{
											if(imgMaxValue < *pData)
												imgMaxValue = *pData;
											if(imgMinValue > *pData)
												imgMinValue = *pData;
										}
										if(fabs(imgMaxValue) > fabs(imgMinValue))
										{
											imgRange = fabs(imgMaxValue);
										}
										else
										{
											imgRange = fabs(imgMinValue);
										}
										imgScaleFactor = imgRange/32767.0;
										pData = (int*)pImgData->data();
										short* pShortDestData = (short*)pDestData;
										for(unsigned int j = 0; j < iFramesize/4; j++, pData++,pShortDestData++)
										{
											*pShortDestData = (short)floor(*pData*32767.0/imgRange);
											
										}
										D("Scalefactor: %f", imgScaleFactor);
										D("Scale factor in header: %f", pSrcImgSubHeader->scaleFactor());
										cout << "Image min: " << imgMinValue << ", image max: " << imgMaxValue << endl;
										delete pImgData;
										pImgData = new QByteArray(pDestData,iFramesize/2);
										delete [] pDestData;
									}
									break;
									case CConcordeMainHeader::IntelFloat:
									case CConcordeMainHeader::IEEEFloat:
									{
										float* pData = (float*)pImgData->data();
										pDestData = new char[iFramesize/2];
										for(unsigned int j = 0; j < iFramesize/4; j++, pData++)
										{
											if(imgMaxValue < *pData)
												imgMaxValue = *pData;
											if(imgMinValue > *pData)
												imgMinValue = *pData;
										}
										if(fabs(imgMaxValue) > fabs(imgMinValue))
										{
											imgRange = fabs(imgMaxValue);
										}
										else
										{
											imgRange = fabs(imgMinValue);
										}
										imgScaleFactor = imgRange/32767.0;
										pData = (float*)pImgData->data();
										short* pShortDestData = (short*)pDestData;
										for(unsigned int j = 0; j < iFramesize/4; j++, pData++,pShortDestData++)
										{
											*pShortDestData = (short)floor(*pData*32767.0/imgRange);
											
										}
										D("Scalefactor: %f", imgScaleFactor);
										D("Scale factor in header: %f", pSrcImgSubHeader->scaleFactor());
										cout << "Image min: " << imgMinValue << ", image max: " << imgMaxValue << endl;
										delete pImgData;
										pImgData = new QByteArray(pDestData,iFramesize/2);
										delete [] pDestData;
									}
									break;
									default:
									{
										cout << "Error: Could not handle datatype of micropet image." << endl;
										bUnknownDataType = true;
									}
									break;
								}
								if(!bUnknownDataType)
								{
									CECAT7SubHeaderImage* pEcat7ImgSubHeader;
									pEcat7ImgSubHeader = static_cast<CECAT7SubHeaderImage*>(ecat7Image.createEmptySubHeader());
									pEcat7ImgSubHeader->setData_Type(CECATSubHeader::SunShort);
									pEcat7ImgSubHeader->convertFrom(pSrcImgSubHeader, pSrcImageHeader);

									pEcat7ImgSubHeader->setScale_Factor(pSrcImgSubHeader->scaleFactor()*imgScaleFactor);
									if(fabs(imgMaxValue) > fabs(imgMinValue))
									{
										pEcat7ImgSubHeader->setImage_Min((short)floor(imgMinValue*imgScaleFactor));
										pEcat7ImgSubHeader->setImage_Max(32767);
									}
									else
									{
										pEcat7ImgSubHeader->setImage_Max((short)ceil(imgMaxValue*imgScaleFactor));
										pEcat7ImgSubHeader->setImage_Min(-32768);
									}
									ecat7Image.writeSubHeader(*pEcat7ImgSubHeader, i+1);
									ecat7Image.writeMatrix(*pImgData,i+1);
									delete pEcat7ImgSubHeader;
								}
								delete pImgData;
								delete pSrcImgSubHeader;
								if(bUnknownDataType)
									break;
							}
						}
						pSrcImageVolume->close();
						if(!bUnknownDataType)
						{
							ecat7Image.writeMainHeader(*pEcat7ImgHeader);
							bResult = true;
						}
						ecat7Image.close();
						delete pEcat7ImgHeader;
					}
					delete pSrcImageHeader;
				}
			}
			delete pSrcImageVolume;
		}
	}
	return bResult;
}
