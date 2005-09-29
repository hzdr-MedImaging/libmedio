//! @file main.cpp
//! @author Hagen Moelle
//! @date 11/13/2004

#include <CECATFile.h>
#include <CECAT7MainHeader.h>
#include <CECAT7SubHeaderImage.h>
#include <CHeaderConcorde.h>
#include <CConcordeImage.h>
#include <CMedIOData.h>
#include <CMedIODataFactory.h>
#include <QString>
#include <math.h>
#include <iostream>

using namespace std;

int main( int argc, char ** argv ) 
{
	if(argc == 3)
    	{
		QByteArray* data = new QByteArray();
		QString name(argv[1]);
		QString StoreFileName(argv[2]);
		CMedIOData* ImageVolume = CMedIODataFactory::create(name);
		if(ImageVolume == NULL)
			return 0;
		ImageVolume->open(QIODevice::ReadOnly);
		CHeaderConcorde* head = (CHeaderConcorde*)ImageVolume->header();
		if(head == NULL)
			return 0;

		CECATFile e7image(StoreFileName,CECATMainHeader::ECAT7_Volume16);
		e7image.open(QIODevice::WriteOnly);	
		CECAT7MainHeader* e7_header = (CECAT7MainHeader*)e7image.createEmptyMainHeader();
		
		*e7_header = *e7_header;
	
		for(int i = 0; i < head->numframes(); i++)
		{
			data = ((CConcordeImage*)ImageVolume)->getMatrix(i+1,0,0,0,0);
			
			unsigned int framesize = head->getImageFrameSize();
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
		
			cout << "Scalefactor" << scale_factor << endl;
			cout << head->frame(i+1)->scalefactor() << endl;
			
			delete data;
			data = new QByteArray(byte_image,framesize/2);
			delete byte_image;
			
			CECAT7SubHeaderImage* e7_subheader;
			e7_subheader = (CECAT7SubHeaderImage*)e7image.createEmptySubHeader();
			*e7_subheader = *head;

			e7_subheader->setScale_Factor(head->frame(i+1)->scalefactor()*scale_factor);
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
			e7_subheader->setFrame_Duration((int)(head->frame(i+1)->frameduration()*1000.0));
			e7_subheader->setFrame_Start_Time((int)(head->frame(i+1)->framestart()*1000.0));
			e7_subheader->setDecay_Corr_Fctr(head->frame(i+1)->decaycorrection());
			e7image.writeSubHeader(*e7_subheader, i+1);
			e7image.writeMatrix(*data,i+1);
		}
		ImageVolume->close();
		e7image.writeMainHeader(*e7_header);
		e7image.close();
	}
	else
	{
		cout << "no input or outputfilename given" << endl;			
		return 0;
	}
	return 0;
}
