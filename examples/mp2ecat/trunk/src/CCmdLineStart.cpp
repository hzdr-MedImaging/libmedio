//! @file CCmdLineStart.cpp
//! @brief contains the implementation of the class CCmdLineStart
//! @author Hagen Moelle
//! @date 11/13/2004

#include "CCmdLineStart.h"

#include <CECATFile.h>
#include <CECAT7MainHeader.h>
#include <CECAT7SubHeaderImage.h>
#include <CConcordeMainHeader.h>
#include <CConcordeImage.h>
#include <CMedIOData.h>
#include <CMedIODataFactory.h>
#include <QString>
#include <math.h>
#include <iostream>
#include <rtdebug.h>
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>

using namespace std;

//  Class: CCmdLineStart
//  Constructor: CCmdLineStart
//!
//! constructs a CCmdLineStart object
//!
////////////////////////////////////////////////////////////////////////////////
CCmdLineStart::CCmdLineStart()
	: m_pGetOpt(NULL)
{
	// let us generate our GetOpt object which we are
	// processing on.
	m_pGetOpt = new CGetOpt();
	
	CGetOptCategory* corrgCat = m_pGetOpt->addCategory("General");
	corrgCat->addOption(CGetOptOption::String, 'i', "microPET file (*.img) which should be converted to ECAT format", "input");
	corrgCat->addOption(CGetOptOption::String, 'o', "ECAT image file (*.v) to which microPET file is converted", "output");
	corrgCat->addOption(CGetOptOption::String, 'n', "name of patient in ECAT file", "name");
	
	//m_sInputFileName = "";
	//m_sOutputFileName = "";
	//m_sPatientName = "";
	
}

//  Class: CCmdLineStart
//  Method: parse
//!
//! This method parses the commandline. It seperates the commandline into
//! several commands. (FBP2D, OSMAPOSL, SEQUENCE) 
//! 
//! @param argc: number of parameters
//! @param argv: list of parameters
//!
//! @return true if parsing is successful
////////////////////////////////////////////////////////////////////////////////
bool CCmdLineStart::parse(int argc, char* argv[])
{
	m_pGetOpt->parse(argc, argv);
	return true;
}

//  Class: CCmdLineStart
//  Method: processOSMAPOSL
//!
//! This method processes a OSMAPOSL command from the commandline.  
//! 
//! @return error message
////////////////////////////////////////////////////////////////////////////////
QString CCmdLineStart::process(int argc, char* argv[])
{

	ENTER();
	QString errorMsg;
	bool foundError = false;

	parse(argc, argv);

	if(errorMsg.isNull()) errorMsg = setInputFileName();
	if(errorMsg.isNull()) errorMsg = setOutputFileName();
	if(errorMsg.isNull()) errorMsg = setPatientName();
	
	if(errorMsg.isNull())
	{
		convertFile();
	}
	

	if(foundError || errorMsg.isNull() == false)
	{
		if(errorMsg.isNull() == false)
			std::cerr << argv[0] << ": " << errorMsg.ascii() << std::endl;

		std::cout << std::endl;
		std::cout << "Usage: " << argv[0] << " -i <input file name> -o <output file name> [-n patient name]" << std::endl << std::endl;
		std::cout << std::endl;

		m_pGetOpt->showAllOptions();

		return false;
	}
	RETURN(foundError);
	return errorMsg;
}

//  Class: CCmdLineStart
//  Method: setSinogramFile
//!
//! This method sets the sinogram in reconstruction object.  
//! 
//! @return error message
QString CCmdLineStart::setInputFileName(bool mustExist)
{
	QString errorMsg;

	char* pInputFile = m_pGetOpt->getValueOption('i');
	if(pInputFile)
	{
		if(QFileInfo(pInputFile).exists() == true)
		{
			m_sInputFileName = QString(pInputFile);
		}
		else errorMsg = "specified input file (*.img) doesn't exist.";
	}
	else if(mustExist == true)
	{
		errorMsg = "specification of input file is missing.";
	}

	return errorMsg;
}

//  Class: CCmdLineStart
//  Method: setAttenuationFile
//!
//! This method sets the attenuation in reconstruction object.  
//! 
//! @return error message
QString CCmdLineStart::setOutputFileName(bool mustExist)
{
	QString errorMsg;

	char* pOutputFile = m_pGetOpt->getValueOption('o');
	if(pOutputFile)
	{
		//if(QFileInfo(pOutputFile).isWritable() == true)
		//{
			m_sOutputFileName = QString(pOutputFile);
		//}
		//else errorMsg = "specified output file (*.v) can not be created.";
	}
	else if(mustExist == true)
	{
		errorMsg = "specification of output file is missing.";
	}

	return errorMsg;
}

//  Class: CCmdLineStart
//  Method: setNormalisationFile
//!
//! This method sets the normalisation in reconstruction object.  
//! 
//! @return error message
QString CCmdLineStart::setPatientName(bool mustExist)
{
	QString errorMsg;

	char* pPatientName = m_pGetOpt->getValueOption('n');
	if(pPatientName)
	{
		m_sPatientName = QString(pPatientName);
	}
	else if(mustExist == true)
	{
		errorMsg = "specification of patient name is missing.";
	}

	return errorMsg;
}

bool CCmdLineStart::convertFile()
{
	int result = 0;

	QString name(m_sInputFileName);
	QString StoreFileName(m_sOutputFileName);
	
	cout << "Opening Image file: " << name.toAscii().data() << endl;
	
	//create new CMedIOData object from file
	CMedIOData* ImageVolume = CMedIODataFactory::create(name);
	if(ImageVolume == NULL)
	{
		cout << "Image source file does not exist." << endl;
		result = 1;
	}
	else
	{
		if(!ImageVolume->open(QIODevice::ReadOnly) || !(ImageVolume->rtti() == CMedIOData::ConcordeMicropet))
			result = 1;
		else
		{
			CConcordeMainHeader* head = NULL;
			if(!((CConcordeFile*)ImageVolume)->readMainHeader(head) || !(head->fileType() == CConcordeMainHeader::Image))
				result = 1;
			else
			{
				W("Creating empty ecat image: %s", StoreFileName.toAscii().data());
				CECATFile e7image(StoreFileName, CECATMainHeader::ECAT7_Volume16);
				e7image.open(QIODevice::WriteOnly);	
				CECAT7MainHeader* e7_header = (CECAT7MainHeader*)e7image.createEmptyMainHeader();
				
				*static_cast<CMedIOHeader*>(e7_header) = *static_cast<CMedIOHeader*>(head);
				e7_header->setPatient_Name(m_sPatientName.toAscii().data());

				for(int i = 0; i < head->totalFrames(); i++)
				{
					QByteArray* data = NULL;
					CConcordeFrameHeader* subHeader = NULL;
					if(!((CConcordeFile*)ImageVolume)->readSubHeader(subHeader, i+1) || !((CConcordeFile*)ImageVolume)->readMatrix(data, i+1))
					{
						result = 1;
						delete data;
						delete subHeader;
						break;
					}
					else
					{	
						unsigned int framesize = head->frameSize();
						cout << "Framesize: " << framesize << endl;
						float* b = (float*)data->data();
						
						char* byte_image = new char[framesize/2];
						
						float max = 0.0F;
						float min = 10000.0F;
						float range;
						for(unsigned int j = 0; j < framesize/4; j++, b++)
						{
							if(max < *b)
								max = *b;
							if(min > *b)
								min = *b;
						}
						
						if(fabs(max) > fabs(min))
						{
							range = fabs(max);
						}
						else
						{
							range = fabs(min);
						}
						
						float scale_factor = range/32767.0;
						b = (float*)data->data();
						short* l = (short*)byte_image;
						for(unsigned int j = 0; j < framesize/4; j++, b++,l++)
						{
							*l = (short) floor(*b*32767.0/range);
							
						}
					
						cout << "Scalefactor: " << scale_factor << endl;
						cout << "Scale factor in header: " << subHeader->scaleFactor() << endl;

						delete data;
						data = new QByteArray(byte_image,framesize/2);
						delete byte_image;
						
						CECAT7SubHeaderImage* e7_subheader;
						e7_subheader = (CECAT7SubHeaderImage*)e7image.createEmptySubHeader();
						e7_subheader->setData_Type(CECATSubHeader::SunShort);
						*static_cast<CMedIOHeader*>(e7_subheader) = *static_cast<CMedIOHeader*>(subHeader);
						 
						e7_subheader->setScale_Factor(subHeader->scaleFactor()*scale_factor);
						if(fabs(max) > fabs(min))
						{
							e7_subheader->setImage_Min((short)ceil(min*scale_factor));
							e7_subheader->setImage_Max(32767);
						}
						else
						{
							e7_subheader->setImage_Max((short)ceil(min*scale_factor));
							e7_subheader->setImage_Min(-32768);
						}
						e7image.writeSubHeader(*e7_subheader, i+1);
						e7image.writeMatrix(*data,i+1);
						delete data;
						delete subHeader;
					}
				}
				ImageVolume->close();
				e7image.writeMainHeader(*e7_header);
				e7image.close();	
			}
		}
	}
	return result;
}
