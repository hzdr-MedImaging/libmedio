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
#include <qstring.h>
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
		ImageVolume->open(IO_ReadOnly);
		CHeaderConcorde* head = (CHeaderConcorde*)ImageVolume->header();
		if(head == NULL)
			return 0;

		CECATFile e7image(StoreFileName,CECATMainHeader::ECAT7_Volume16);
		e7image.open(IO_WriteOnly);	
		CECAT7MainHeader* e7_header = (CECAT7MainHeader*)e7image.createEmptyMainHeader();
		
		e7_header->setOriginal_File_Name(head->filename().ascii());
		
		e7_header->setSystem_Type((short)head->model());
		e7_header->setScan_Start_Time(head->scantime());
		e7_header->setIsotope_Name(head->isotope().ascii());
		e7_header->setIsotope_Halflife(head->isotopehalftime());
		e7_header->setBed_Elevation(head->frame(1)->verticalbedoffset());
		e7_header->setDistance_Scanned(head->radialfov());
		e7_header->setTransaxial_FOV(head->radialfov());
		e7_header->setCoin_Samp_Mode(CECAT7MainHeader::NetTrues);
		e7_header->setCalibration_Factor(head->calibrationfactor());
		if(head->calibrationfactor() == 1.0F)
			e7_header->setCalibration_Units(CECAT7MainHeader::Uncalibrated);
		else
			e7_header->setCalibration_Units(CECAT7MainHeader::Calibrated);
		e7_header->setStudy_Type(head->study().ascii());
		e7_header->setPatient_ID(head->subjectidentifier().ascii());
		e7_header->setPatient_Name(head->subjectidentifier().ascii());
		e7_header->setPatient_Sex(CECAT7MainHeader::Sex_Male);
		e7_header->setPatient_Height(head->subjectlength());
		e7_header->setPatient_Weight(head->subjectweight());
		e7_header->setPhysician_Name(head->investigator().ascii());
		e7_header->setOperator_Name(head->Operator().ascii());
		e7_header->setStudy_Description(head->studyidentifier().ascii());
		if(head->acquisitionmode() == CHeaderConcorde::Emission)
			e7_header->setAcquisition_Type(CECAT7MainHeader::StaticEmission);
		else
			e7_header->setAcquisition_Type(CECAT7MainHeader::DynamicEmission);
		switch(head->subjectorientation())
		{
			case 0: e7_header->setPatient_Orientation(CECAT7MainHeader::FF_Prone); break;
			case 1: e7_header->setPatient_Orientation(CECAT7MainHeader::FF_Prone); break;
			case 2: e7_header->setPatient_Orientation(CECAT7MainHeader::HF_Prone); break;
			case 3: e7_header->setPatient_Orientation(CECAT7MainHeader::FF_Supine); break;
			case 4: e7_header->setPatient_Orientation(CECAT7MainHeader::HF_Supine); break;
			case 5: e7_header->setPatient_Orientation(CECAT7MainHeader::FF_Right); break;
			case 6: e7_header->setPatient_Orientation(CECAT7MainHeader::HF_Right); break;
			case 7: e7_header->setPatient_Orientation(CECAT7MainHeader::FF_Left); break;
			case 8: e7_header->setPatient_Orientation(CECAT7MainHeader::HF_Left); break;
		}
		e7_header->setFacility_Name(head->institution().ascii());
		e7_header->setNum_Planes(head->zdimension());
		e7_header->setNum_Frames(head->numframes());
		e7_header->setLwr_True_Thres((short)head->lld());
		e7_header->setUpr_True_Thres((short)head->uld());
		e7_header->setBranching_Fraction(head->isotopebranchingfraction());
		e7_header->setNum_Gates(1);
		e7_header->setNum_Bed_Pos(0);
		e7_header->setInit_Bed_Position(head->frame(1)->bedoffset());
		e7_header->setDose_Start_Time(head->injectiontime());
		e7_header->setDosage(head->dose());
		
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
			cout << head->frame(i)->scalefactor() << endl;
			char* tmp = data->data();
			data->resetRawData(tmp,framesize);
			delete [] tmp;
		
			data->setRawData(byte_image,framesize/2);
			
			CECAT7SubHeaderImage* e7_subheader;
			e7_subheader = (CECAT7SubHeaderImage*)e7image.createEmptySubHeader();
			e7_subheader->setData_Type(CECATSubHeader::SunShort);
			e7_subheader->setNum_Dimensions(3);
			e7_subheader->setX_Dimension(head->xdimension());
			e7_subheader->setY_Dimension(head->ydimension());
			e7_subheader->setZ_Dimension(head->zdimension());
			e7_subheader->setX_Pixel_Size(head->pixelsize());
			e7_subheader->setY_Pixel_Size(head->pixelsize());
			e7_subheader->setZ_Pixel_Size(head->axialplanesize());
			e7_subheader->setRecon_Zoom(1.0);
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
			e7_subheader->setNum_R_Elements(head->ydimension());
			e7_subheader->setNum_Angles(head->xdimension());
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
