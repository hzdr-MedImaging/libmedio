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
	//QListIterator<CHeaderConcordeFrame*> nums_iter(frames);
	
	//while(nums_iter.hasNext())
	//{
	//	CHeaderConcordeFrame* tmp = (CHeaderConcordeFrame*)nums_iter.next();
	//	delete tmp;
	//	tmp = NULL;
	//}
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
	bool result = false;
	//unset function should be used !!!
	ENTER();
	if(!m_pMedIOData)
	{
		D("Something wrong with m_pMedIOData");
		LEAVE();
		return false;	
	}
	QString File = m_pMedIOData->fileName() + ".hdr";
	
	result = load(File);
	RETURN(result);
	return result;
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
	ENTER();
	D("Start Parsing File %s", File.toAscii().data());
	if(Parser.parse(File))
	{
		D("Parsing of mainheader seems to be ok");
		//for(int i = 0; i < m_Data.total_frames; i++)
		//{
		//	CHeaderConcordeFrame* frame = new CHeaderConcordeFrame(File, i);
		//	frames.append(frame);
		//}
		
	}
	else
	{
		D("Headerfile %s nicht gefunden !", File.toAscii().data());
		return false;
	}
	RETURN(true);
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
//CHeaderConcordeFrame* CHeaderConcorde::frame(int i)
//{
//	if(frames.size() < i || i < 1)
//		return frames[0];
//	return frames[i-1];
//}

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
	ENTER();
	if(src.headerFormat() == CMedIOHeader::ConcordeMicropet)
	{
		//TODO: add CHeaderConcordeFrame to CMedIOHeader and check for it
		W("TODO: add CHeaderConcordeFrame to CMedIOHeader and check for it");
		CHeaderConcorde* head = (CHeaderConcorde*)&src;
		m_Data.model = head->model();
		m_Data.institution = head->institution();
		m_Data.study = head->study();
		m_Data.file_name = head->fileName();
		m_Data.file_type = head->fileType();
		m_Data.acquisition_mode = head->acquisitionMode();
		m_Data.bed_motion = head->bedMotion();
		m_Data.total_frames = head->totalFrames();
		m_Data.isotope = head->isotope();
		m_Data.isotope_half_time = head->isotopeHalfTime();
		m_Data.isotope_branching_fraction = head->isotopeBranchingFraction();
		m_Data.transaxial_crystals_per_block = head->transaxialCrystalsPerBlock();
		m_Data.axial_crystals_per_block = head->axialCrystalsPerBlock();
		m_Data.intrinsic_crystal_offset = head->intrinsicCrystalOffset();
		m_Data.axial_blocks = head->axialBlocks();
		m_Data.axial_crystal_pitch = head->axialCrystalPitch();
		m_Data.radius = head->radius();
		m_Data.radial_fov = head->radialFov();
		m_Data.src_radius = head->srcRadius();
		m_Data.src_cm_per_rev = head->srcCmPerRev();
		m_Data.tx_src_type = head->txSrcType();
		m_Data.transaxial_bin_size = head->transaxialBinSize();
		m_Data.axial_plane_size = head->axialPlaneSize();
		m_Data.pixel_size = head->pixelSize();
		m_Data.lld = head->lld();
		m_Data.uld = head->uld();
		m_Data.data_type = head->dataType();
		m_Data.data_order = head->dataOrder();
		m_Data.span = head->span();
		m_Data.ring_difference = head->ringDifference();
		m_Data.number_of_dimensions = head->numberOfDimensions();
		m_Data.x_dimension = head->xDimension();
		m_Data.y_dimension = head->yDimension();
		m_Data.z_dimension = head->zDimension();
		m_Data.w_dimension = head->wDimension();
		if(m_Data.w_dimension > 1)
			for(int i = 0; i < m_Data.w_dimension; i++)
				m_Data.delta_elements[i] = head->deltaElements(i);
		m_Data.deadtime_correction_applied = head->deadtimeCorrectionApplied();
		m_Data.decay_correction_applied = head->decayCorrectionApplied();
		m_Data.normalization_applied = head->normalizationApplied();
		m_Data.attenuation_applied = head->attenuationApplied();
		m_Data.scatter_correction = head->scatterCorrection();
		m_Data.arc_correction_applied = head->arcCorrectionApplied();
		m_Data.calibration_factor = head->calibrationFactor();
		m_Data.calibration_branching_fraction = head->calibrationBranchingFraction();
		m_Data.number_of_singles_rates = head->numberOfSinglesRates();
		m_Data.investigator = head->investigator();
		m_Data.Operator = head->Operator();
		m_Data.study_identifier = head->studyIdentifier();
		m_Data.scan_time = head->scanTime();
		m_Data.injected_compound = head->injectedCompound();
		m_Data.dose_units = head->doseUnits();
		m_Data.dose = head->dose();
		m_Data.injection_time = head->injectionTime();
		m_Data.injection_decay_correction = head->injectionDecayCorrection();
		m_Data.subject_identifier = head->subjectIdentifier();
		m_Data.subject_genus = head->subjectGenus();
		m_Data.subject_orientation = head->subjectOrientation();
		m_Data.subject_length_units = head->subjectLengthUnits();
		m_Data.subject_length = head->subjectLength();
		m_Data.subject_weight_units = head->subjectWeightUnits();
		m_Data.subject_weight = head->subjectWeight();
		m_Data.subject_phenotype = head->subjectPhenotype();
		m_Data.study_model = head->studyModel();
		m_Data.anesthesia = head->anesthesia();
		m_Data.analgesia = head->analgesia();
		m_Data.other_drugs = head->otherDrugs();
		m_Data.food_access = head->foodAccess();
		m_Data.water_access = head->waterAccess();
	}
	RETURN(this);
	return *this;
}
