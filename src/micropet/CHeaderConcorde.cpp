//! @file CHeaderConcorde.cpp
//! @brief contains the implementation of the class CHeaderConcorde
//! @author Hagen Moelle
//! @date 11/13/2004

#include "CHeaderConcorde.h"
#include "CHeaderConcordeFrame.h"
#include "CMedIOHeader.h"
#include "CMedIOData.h"

#include <rtdebug.h>
#include <QDateTime>
#include <QString>
#include <QList>

//  Class: CHeaderConcorde
//  Constructor: CHeaderConcorde
//!
//! constructs a CHeaderConcorde object
//!
//! @param file: complete path to file holding header
////////////////////////////////////////////////////////////////////////////////
CHeaderConcorde::CHeaderConcorde(QString File)
{ 
	init();
	setDefaults();
	if(!this->load(File))
	{
		D("Something is wrong with the headerfile");
	}
	else
		D("Everything ok");
}

//  Class: CHeaderConcorde
//  Constructor: CHeaderConcorde
//!
//! constructs a CHeaderConcorde object
//!
////////////////////////////////////////////////////////////////////////////////
CHeaderConcorde::CHeaderConcorde(CConcordeFile* file)
{
	//initialise all keys which should be searched for in
	//header file
	D("Setting reference to CMedIOData object");
	m_pMedIOData = file;
	D("Initialising all keyvalues");
	init();
	D("Setting defaults");
	setDefaults();
	D("Trying to load headerfile");
	if(!this->load())
	{
		D("Something is wrong with the headerfile");
	}
	else
		D("Everything ok");
}

//  Class: CHeaderConcorde
//  Destructor:  CHeaderConcorde
//!
//! destructucts a CHeaderConcorde object.
//! 
////////////////////////////////////////////////////////////////////////////////
CHeaderConcorde::~CHeaderConcorde()
{
	ENTER();
	QListIterator<CHeaderConcordeFrame*> nums_iter(frames);
	
	while(nums_iter.hasNext())
	{
		CHeaderConcordeFrame* tmp = (CHeaderConcordeFrame*)nums_iter.next();
		delete tmp;
		tmp = NULL;
	}
	LEAVE();
}

//  Class: CHeaderConcorde
//  Method: setDefaults
//!
//! This methods sets some default values in the header. That minimizes 
//! the occurance of errors due to undefined values.
//!
////////////////////////////////////////////////////////////////////////////////
void CHeaderConcorde::setDefaults()
{
	m_Data.model = 0;
	m_Data.institution = "FZ Rossendorf"; 
	m_Data.file_type = 0;
	m_Data.acquisition_mode = 0;
	m_Data.bed_motion = 0;
	m_Data.total_frames = 0;
	m_Data.transaxial_crystals_per_block = 8;
	m_Data.axial_crystals_per_block = 8;
	m_Data.intrinsic_crystal_offset = 4;
	m_Data.transaxial_blocks = 42;
	m_Data.axial_blocks = 4;
	m_Data.transaxial_crystal_pitch = 0.2444F;
	m_Data.axial_crystal_pitch = 0.2423F;
	m_Data.radius = 13.0673F;
	m_Data.radial_fov = 12.067F;
	m_Data.src_radius = 11.2F;
	m_Data.src_cm_per_rev = 0.05F;
	m_Data.tx_src_type = 0;
	m_Data.transaxial_bin_size = 0.1222F;
	m_Data.axial_plane_size = 0.12115F;
	m_Data.lld = 350.0F;
	m_Data.uld = 750.0F;
	m_Data.data_type = 0;
	m_Data.data_order = 1;

	m_Data.span = 3;
	m_Data.ring_difference = 31;
	m_Data.number_of_dimensions = 4;
	m_Data.x_dimension = 192;
	m_Data.y_dimension = 168;
	m_Data.z_dimension = 63;
	m_Data.w_dimension = 11;

	m_Data.deadtime_correction_applied = 0;
	m_Data.decay_correction_applied = 0;
	m_Data.normalization_applied = 0;
	m_Data.attenuation_applied = 0;
	m_Data.scatter_correction = 0;
	m_Data.arc_correction_applied = 0;
	m_Data.x_offset = 0.0F;
	m_Data.y_offset = 0.0F;
	m_Data.zoom = 1.0F;
	m_Data.pixel_size = 0.0F;
	m_Data.calibration_units = 0;
	m_Data.calibration_factor = 0.0F;
	m_Data.calibration_branching_fraction = 1.0F;
	m_Data.number_of_singles_rates = 168;
	m_Data.dose_units = 0;
	m_Data.subject_orientation = 0;
	m_Data.subject_length_units = 0;
	m_Data.subject_weight_units = 0;
}

bool CHeaderConcorde::load()
{
	//unset function should be used !!!
	ENTER();
	if(!m_pMedIOData)
	{
		D("Something wrong with m_pMedIOData");
		LEAVE();
		return false;	
	}
	QString File = m_pMedIOData->fileName() + ".hdr";
	LEAVE();
	return load(File);
}

//  Class: CHeaderConcorde
//  Method: load
//!
//! loades the headerinformation.
//!
//! @return true on success otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CHeaderConcorde::load(QString File)
{
	D("Start Parsing File %s", File.toAscii().data());
	if(Parser.parse(File))
	{
		D("Parsing of mainheader seems to be ok");
		for(int i = 0; i < m_Data.total_frames; i++)
		{
			CHeaderConcordeFrame* frame = new CHeaderConcordeFrame(File, i);
			frames.append(frame);
		}
		
	}
	else
	{
		D("Headerfile %s nicht gefunden !", File.toAscii().data());
		return false;
	}
	return true;
}

//  Class: CHeaderConcorde
//  Method: save
//!
//! Saves the headerinformation.
//!
//! @return true on success otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CHeaderConcorde::save() const
{
	return true;
}

CMedIOHeader::Format CHeaderConcorde::headerFormat() const
{
	return CMedIOHeader::ConcordeMicropet;
}

int CHeaderConcorde::rtti()
{
	return CMedIOHeader::ConcordeMicropet;
}

//  Class: CHeaderConcorde
//  Method: getFrameSize
//!
//! @return framesize of a sinogram in bytes
////////////////////////////////////////////////////////////////////////////////
unsigned int CHeaderConcorde::getFrameSize()
{	
	int framesize = 0;
	int typesize = 0;
	//calculate the frame size: 
	//frame_size = number_of_sinograms*x_dimension*y_dimension*data_type_size
	//number_of_dimensions = sum(sinograms) over all delta_elements
	if(m_Data.data_type == 2 | m_Data.data_type == 6)
	{
		typesize = 2;
	}
	else 
		typesize = 4;
	
	if(m_Data.data_type == 0)
	{
		typesize = 1;
	}
	if(m_Data.data_type == 1)
	{
		typesize = 1;
	}
	
	for(int i = 0; i < 11; i++) framesize += m_Data.delta_elements[i];
	framesize = framesize*m_Data.x_dimension*m_Data.y_dimension*typesize;
	return framesize;
}

//  Class: CHeaderConcorde
//  Method: getImageFrameSize
//!
//! @return framesize of a imagevolume in bytes
////////////////////////////////////////////////////////////////////////////////
unsigned int CHeaderConcorde::getImageFrameSize()
{	
	int framesize = 0;
	int typesize = 0;
	//calculate the imageframe size: 
	//framesize = x_dimension*y_dimension*z_dimension*data_type_size
	if(m_Data.data_type == 2 | m_Data.data_type == 6)
	{
		typesize = 2;
	}
	else 
		typesize = 4;
	
	if(m_Data.data_type == 0)
	{
		typesize = 1;
	}
	if(m_Data.data_type == 1)
	{
		typesize = 1;
	}
	
	framesize = m_Data.x_dimension*m_Data.y_dimension*m_Data.z_dimension*typesize;
	return framesize;
}

//  Class: CHeaderConcorde
//  Method: getImageFrameSize
//!
//! access frames starting with frame 1 as first frame -> i = 1
//!
//! @return frame specific header
////////////////////////////////////////////////////////////////////////////////
CHeaderConcordeFrame* CHeaderConcorde::frame(int i)
{
	if(frames.size() < i || i < 1)
		return frames[0];
	return frames[i-1];
}

//  Class: CHeaderConcorde
//  Method: init
//!
//! //initialise all keys which should be searched for in header file
//!
//! @return true on success otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CHeaderConcorde::init()
{
	Parser.addSeparator(" ");
	Parser.addComment("#");
	Parser.addStopSymbol("end_of_header");

	Parser.addKey("model", &m_Data.model);
	Parser.addKey("institution", &m_Data.institution);
	Parser.addKey("study", &m_Data.study);
	Parser.addKey("file_name", &m_Data.file_name);
	Parser.addKey("file_type", &m_Data.file_type);
	Parser.addKey("acquisition_mode", &m_Data.acquisition_mode);
	Parser.addKey("bed_motion", &m_Data.bed_motion);
	Parser.addKey("total_frames", &m_Data.total_frames);
	
	Parser.addKey("isotope", &m_Data.isotope);
	Parser.addKey("isotope_half_time", &m_Data.isotope_half_time);
	Parser.addKey("isotope_branching_fraction", &m_Data.isotope_branching_fraction);
	
	Parser.addKey("transaxial_crystals_per_block", &m_Data.transaxial_crystals_per_block);
	Parser.addKey("axial_crystals_per_block", &m_Data.axial_crystals_per_block);
	Parser.addKey("intrinsic_crystal_offset", &m_Data.intrinsic_crystal_offset);
	Parser.addKey("axial_blocks", &m_Data.axial_blocks);
	Parser.addKey("axial_crystal_pitch", &m_Data.axial_crystal_pitch);
	Parser.addKey("radius", &m_Data.radius);
	Parser.addKey("radial_fov", &m_Data.radial_fov);
	Parser.addKey("src_radius", &m_Data.src_radius);
	Parser.addKey("src_cm_per_rev", &m_Data.src_cm_per_rev);
	Parser.addKey("tx_src_type", &m_Data.tx_src_type);
	Parser.addKey("transaxial_bin_size", &m_Data.transaxial_bin_size);
	Parser.addKey("axial_plane_size", &m_Data.axial_plane_size);
	Parser.addKey("pixel_size", &m_Data.pixel_size);
	Parser.addKey("lld", &m_Data.lld);
	Parser.addKey("uld", &m_Data.uld);
	
	Parser.addKey("data_type", &m_Data.data_type);
	Parser.addKey("data_order", &m_Data.data_order);
	Parser.addKey("span", &m_Data.span);
	Parser.addKey("ring_difference", &m_Data.ring_difference);
	Parser.addKey("number_of_dimensions", &m_Data.number_of_dimensions);
	Parser.addKey("x_dimension", &m_Data.x_dimension);
	Parser.addKey("y_dimension", &m_Data.y_dimension);
	Parser.addKey("z_dimension", &m_Data.z_dimension);
	Parser.addKey("w_dimension", &m_Data.w_dimension);
	Parser.addKey("delta_elements", &m_Data.delta_elements, CIntVector::wrapper_parseKeyValue);	
	
	Parser.addKey("deadtime_correction_applied", &m_Data.deadtime_correction_applied);
	Parser.addKey("decay_correction_applied", &m_Data.decay_correction_applied);
	Parser.addKey("normalization_applied", &m_Data.normalization_applied);
	Parser.addKey("attenuation_applied", &m_Data.attenuation_applied);
	Parser.addKey("scatter_correction", &m_Data.scatter_correction);
	Parser.addKey("arc_correction", &m_Data.arc_correction_applied);
	
	Parser.addKey("calibration_factor", &m_Data.calibration_factor);
	Parser.addKey("calibration_branching_fraction", &m_Data.calibration_branching_fraction);
	Parser.addKey("number_of_singles_rates", &m_Data.number_of_singles_rates);
	
	Parser.addKey("investigator", &m_Data.investigator);
	Parser.addKey("operator", &m_Data.Operator);
	Parser.addKey("study_identifier", &m_Data.study_identifier);
	//Parser.addKey("scan_time", &m_Data.scan_time);
	Parser.addKey("injected_compound", &m_Data.injected_compound);
	Parser.addKey("dose_units", &m_Data.dose_units);
	Parser.addKey("dose", &m_Data.dose);
	//Parser.addKey("injection_time", &m_Data.injection_time);
	Parser.addKey("injection_decay_correction", &m_Data.injection_decay_correction);
	
	Parser.addKey("subject_identifier", &m_Data.subject_identifier);
	Parser.addKey("subject_genus", &m_Data.subject_genus);
	Parser.addKey("subject_orientation", &m_Data.subject_orientation);
	Parser.addKey("subject_length_units", &m_Data.subject_length_units);
	Parser.addKey("subject_length", &m_Data.subject_length);
	Parser.addKey("subject_weight_units", &m_Data.subject_weight_units);
	Parser.addKey("subject_weight", &m_Data.subject_weight);
	Parser.addKey("subject_phenotype", &m_Data.subject_phenotype);
	Parser.addKey("study_model", &m_Data.study_model);
	
	Parser.addKey("anesthesia", &m_Data.anesthesia);
	Parser.addKey("analgesia", &m_Data.analgesia);
	Parser.addKey("other_drugs", &m_Data.other_drugs);
	Parser.addKey("food_access", &m_Data.food_access);
	Parser.addKey("water_access", &m_Data.water_access);
		
	return true;
}

CMedIOHeader& CHeaderConcorde::copyData(const CMedIOHeader& src)
{
	return *this;
}
