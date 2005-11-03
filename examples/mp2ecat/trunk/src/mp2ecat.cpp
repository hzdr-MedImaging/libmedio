//! @file main.cpp
//! @author Hagen Moelle
//! @date 11/13/2004

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

using namespace std;

int main( int argc, char ** argv ) 
{
	int result = 0;
	if(argc == 3)
    	{
		QString name(argv[1]);
		QString StoreFileName(argv[2]);
		
		cout << "Opening Image file: " << argv[1] << endl;
		
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
					CECATFile e7image(StoreFileName, CECATMainHeader::ECAT7_Volume16);
					e7image.open(QIODevice::WriteOnly);	
					CECAT7MainHeader* e7_header = (CECAT7MainHeader*)e7image.createEmptyMainHeader();
					
					*static_cast<CMedIOHeader*>(e7_header) = *static_cast<CMedIOHeader*>(head);
				
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
	}
	else
	{
		cout << "No input or outputfilename given" << endl;			
		result = 1;
	}
	return result;
}
