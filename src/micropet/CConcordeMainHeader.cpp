//! @file CConcordeMainHeader.cpp
//! @brief contains the implementation of the class ConcordeMainHeader
//! @author Hagen Moelle
//! @date 09/10/2005

#include "CConcordeMainHeader.h"
#include "CMedIOHeader.h"
#include "CMedIOData.h"

#include <rtdebug.h>
#include <qdatetime.h>
#include <qstring.h>

//  Class: CConcordeMainHeader
//  Constructor: CConcordeMainHeader
//!
//! constructs a CConcordeMainHeader object
//!
//! @param file: complete path to file holding header
////////////////////////////////////////////////////////////////////////////////
CConcordeMainHeader::CConcordeMainHeader(QString File)
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

//  Class: CConcordeMainHeader
//  Constructor: CConcordeMainHeader
//!
//! constructs a CHeaderConcorde object
//!
////////////////////////////////////////////////////////////////////////////////
CConcordeMainHeader::CConcordeMainHeader(CConcordeFile* file)
{
	//initialise all keys which should be searched for in
	//header file
	D("Setting reference to CMedIOData object");
	m_pMedIOData = file;
	D("Initialising all keyvalues");
	init();
	D("Setting defaults");
	setDefaults();
	//D("Trying to load headerfile");
	//if(!this->load())
	//{
	//	D("Something is wrong with the headerfile");
	//}
	//else
	//	D("Everything ok");
}

//  Class: CConcordeMainHeader
//  Destructor:  CConcordeMainHeader
//!
//! destructucts a CConcordeMainHeader object.
//! 
////////////////////////////////////////////////////////////////////////////////
CConcordeMainHeader::~CConcordeMainHeader()
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

int	CConcordeMainHeader::model(void) const
{
	return m_Data.model;
}

QString	CConcordeMainHeader::institution(void) const
{
	return m_Data.institution;
}

QString	CConcordeMainHeader::study(void) const
{
	return m_Data.study;
}

QString	CConcordeMainHeader::fileName(void) const
{
	return m_Data.file_name;
}

CConcordeMainHeader::FileType CConcordeMainHeader::fileType(void) const
{
	return static_cast<FileType>(m_Data.file_type);
}

CConcordeMainHeader::AquisitionMode CConcordeMainHeader::acquisitionMode(void) const
{
	return static_cast<AquisitionMode>(m_Data.acquisition_mode);
}

CConcordeMainHeader::BedMotion CConcordeMainHeader::bedMotion(void) const
{
	return static_cast<BedMotion>(m_Data.bed_motion);
}

int CConcordeMainHeader::totalFrames(void) const
{
	return m_Data.total_frames;
}

QString CConcordeMainHeader::isotope(void) const
{
	return m_Data.isotope;
}

float CConcordeMainHeader::isotopeHalfTime(void) const
{
	return m_Data.isotope_half_time;
}

float CConcordeMainHeader::isotopeBranchingFraction(void) const
{
	return m_Data.isotope_branching_fraction;
}

int CConcordeMainHeader::transaxialCrystalsPerBlock(void) const
{
	return m_Data.transaxial_crystals_per_block;
}

int CConcordeMainHeader::axialCrystalsPerBlock(void) const
{
	return m_Data.axial_crystals_per_block;
}

int CConcordeMainHeader::intrinsicCrystalOffset(void) const
{
	return m_Data.intrinsic_crystal_offset;
}

int CConcordeMainHeader::axialBlocks(void) const
{
	return m_Data.axial_blocks;
}

float CConcordeMainHeader::axialCrystalPitch(void) const
{
	return m_Data.axial_crystal_pitch;
}

float CConcordeMainHeader::radius(void) const
{
	return m_Data.radius;
}

float CConcordeMainHeader::radialFov(void) const
{
	return m_Data.radial_fov;
}

float CConcordeMainHeader::srcRadius(void) const
{
	return m_Data.src_radius;
}

float CConcordeMainHeader::srcCmPerRev(void) const
{
	return m_Data.src_cm_per_rev;
}

CConcordeMainHeader::SourceType CConcordeMainHeader::txSrcType(void) const
{
	return static_cast<SourceType>(m_Data.tx_src_type);
}

float CConcordeMainHeader::transaxialBinSize(void) const
{
	return m_Data.transaxial_bin_size;
}

float CConcordeMainHeader::axialPlaneSize(void) const
{
	return m_Data.axial_plane_size;
}

float CConcordeMainHeader::lld(void) const
{
	return m_Data.lld;
}

float CConcordeMainHeader::uld(void) const
{
	return m_Data.uld;
}

CConcordeMainHeader::DataType CConcordeMainHeader::dataType(void) const
{
	return static_cast<DataType>(m_Data.data_type);
}

CConcordeMainHeader::DataOrder CConcordeMainHeader::dataOrder(void) const
{
	return static_cast<DataOrder>(m_Data.data_order);
}

int CConcordeMainHeader::span(void) const
{
	return m_Data.span;
}

int CConcordeMainHeader::ringDifference(void) const
{
	return m_Data.ring_difference;
}
		
int CConcordeMainHeader::numberOfDimensions(void) const
{
	return m_Data.number_of_dimensions;
}

int CConcordeMainHeader::xDimension(void) const
{
	return m_Data.x_dimension;
}

int CConcordeMainHeader::yDimension(void) const
{
	return m_Data.y_dimension;
}

int CConcordeMainHeader::zDimension(void) const
{
	return m_Data.z_dimension;
}

int CConcordeMainHeader::wDimension(void) const
{
	return m_Data.w_dimension;
}

int CConcordeMainHeader::deltaElements(int i) const
{
	return m_Data.delta_elements[i];
}

CConcordeMainHeader::DeadTimeCorrectionApplied CConcordeMainHeader::deadtimeCorrectionApplied(void) const
{
	return static_cast<DeadTimeCorrectionApplied>(m_Data.deadtime_correction_applied);
}

CConcordeMainHeader::DecayCorrectionApplied CConcordeMainHeader::decayCorrectionApplied(void) const
{
	return static_cast<DecayCorrectionApplied>(m_Data.decay_correction_applied);
}

CConcordeMainHeader::NormalizationApplied CConcordeMainHeader::normalizationApplied(void) const
{
	return static_cast<NormalizationApplied>(m_Data.normalization_applied);
}

CConcordeMainHeader::AttenuationApplied CConcordeMainHeader::attenuationApplied(void) const
{
	return static_cast<AttenuationApplied>(m_Data.attenuation_applied);
}

CConcordeMainHeader::ScatterCorrection CConcordeMainHeader::scatterCorrection(void) const
{
	return static_cast<ScatterCorrection>(m_Data.scatter_correction);
}

CConcordeMainHeader::ArcCorrectionApplied CConcordeMainHeader::arcCorrectionApplied(void) const
{
	return static_cast<ArcCorrectionApplied>(m_Data.arc_correction_applied);
}

float CConcordeMainHeader::pixelSize(void) const
{
	return m_Data.pixel_size;
}

CConcordeMainHeader::CalibrationUnits CConcordeMainHeader::calibrationUnits(void) const
{
	return static_cast<CalibrationUnits>(m_Data.calibration_units);
}

float CConcordeMainHeader::calibrationFactor(void) const
{
	return m_Data.calibration_factor;
}

float CConcordeMainHeader::calibrationBranchingFraction(void) const
{
	return m_Data.calibration_branching_fraction;
}

int CConcordeMainHeader::numberOfSinglesRates(void) const
{
	return m_Data.number_of_singles_rates;
}

QString CConcordeMainHeader::investigatorName(void) const
{
	return m_Data.investigator;
}

QString CConcordeMainHeader::operatorName(void) const
{
	return m_Data.Operator;
}

QString CConcordeMainHeader::studyIdentifier(void) const
{
	return m_Data.study_identifier;
}

long CConcordeMainHeader::scanTime(void) const
{
	return m_Data.scan_time.toTime_t();
}

QString CConcordeMainHeader::injectedCompound(void) const
{
	return m_Data.injected_compound;
}

CConcordeMainHeader::DoseUnits CConcordeMainHeader::doseUnits(void) const
{
	return static_cast<DoseUnits>(m_Data.dose_units);
}

float CConcordeMainHeader::dose(void) const
{
	return m_Data.dose;
}

long CConcordeMainHeader::injectionTime(void) const
{
	return m_Data.injection_time.toTime_t();
}

float CConcordeMainHeader::injectionDecayCorrection(void) const
{
	return m_Data.injection_decay_correction;
}

QString CConcordeMainHeader::subjectIdentifier(void) const
{
	return m_Data.subject_identifier;
}

QString CConcordeMainHeader::subjectGenus(void) const
{
	return m_Data.subject_genus;
}

CConcordeMainHeader::SubjectOrientation CConcordeMainHeader::subjectOrientation(void) const
{
	return static_cast<SubjectOrientation>(m_Data.subject_orientation);
}

CConcordeMainHeader::SubjectLengthUnits CConcordeMainHeader::subjectLengthUnits(void) const
{
	return static_cast<SubjectLengthUnits>(m_Data.subject_length_units);
}

float CConcordeMainHeader::subjectLength(void) const
{
	return m_Data.subject_length;
}

CConcordeMainHeader::SubjectWeightUnits CConcordeMainHeader::subjectWeightUnits(void) const
{
	return static_cast<SubjectWeightUnits>(m_Data.subject_weight_units);
}

float CConcordeMainHeader::subjectWeight(void) const
{
	return m_Data.subject_weight;
}

QString CConcordeMainHeader::subjectPhenotype(void) const
{
	return m_Data.subject_phenotype;
}

QString CConcordeMainHeader::studyModel(void) const
{
	return m_Data.study_model;
}

QString CConcordeMainHeader::anesthesia(void) const
{
	return m_Data.anesthesia;
}

QString CConcordeMainHeader::analgesia(void) const
{
	return m_Data.analgesia;
}

QString CConcordeMainHeader::otherDrugs(void) const
{
	return m_Data.other_drugs;
}

QString CConcordeMainHeader::foodAccess(void) const
{
	return m_Data.food_access;
}

QString CConcordeMainHeader::waterAccess(void) const
{
	return m_Data.water_access;
}

QDateTime CConcordeMainHeader::scanTimeQt(void) const
{
	return static_cast<QDateTime>(m_Data.scan_time);
}

QDateTime CConcordeMainHeader::injectionTimeQt(void) const
{
	return static_cast<QDateTime>(m_Data.injection_time);
}

QString CConcordeMainHeader::strSubjectWeightUnits(void) const
{
	QString tmp; 
	switch(m_Data.subject_weight_units)
	{
		case Grams: tmp = "g"; break;
		case Ounces: tmp = "oz"; break;
		case Kilograms: tmp = "kg"; break;
		case Pounds: tmp = "pounds"; break;
		default: tmp = "unknown units"; break;
	}
	return tmp;
}
QString CConcordeMainHeader::strSubjectLengthUnits(void) const
{
	QString tmp; 
	switch(m_Data.subject_length_units)
	{
		case Millimeters: tmp = "mm"; break;
		case Centimeters: tmp = "cm"; break;
		case Inches: tmp = "in"; break;
		default: tmp = "unknown units"; break;
	}
	return tmp;
}
QString CConcordeMainHeader::strDoseUnits(void) const
{
	QString tmp; 
	switch(m_Data.dose_units)
	{
		case mCi: tmp = "mCi"; break;
		case MBq: tmp = "MBq"; break;
		default: tmp = "unknown units"; break;
	}
	return tmp;
}

//mutator methods
void CConcordeMainHeader::setModel(const int value)
{
	 m_Data.model = value;
}

void CConcordeMainHeader::setInstitution(const QString value)
{
	 m_Data.institution = value;
}

void CConcordeMainHeader::setFileName(const QString value)
{
	 m_Data.file_name = value;
}

void CConcordeMainHeader::setFileType(const int value)
{
	 m_Data.file_type = value;
}

void CConcordeMainHeader::setAcquisitionMode(const int value)
{
	 m_Data.acquisition_mode = value;
}

void CConcordeMainHeader::setBedMotion(const int value)
{
	 m_Data.bed_motion = value;
}

void CConcordeMainHeader::setNumFrames(const int value)
{
	 m_Data.total_frames = value;
}

void CConcordeMainHeader::setIsotope(const QString value)
{
	 m_Data.isotope = value;
}

void CConcordeMainHeader::setIsotopeHalftime(const float value)
{
	 m_Data.isotope_half_time = value;
}

void CConcordeMainHeader::setIsotopeBranchingFraction(const float value)
{
	 m_Data.isotope_branching_fraction = value;
}

void CConcordeMainHeader::setTransaxialCrystalsPerBlock(const int value)
{
	m_Data.transaxial_crystals_per_block = value;
}

void CConcordeMainHeader::setAxialCrystalsPerBlock(const int value)
{
	m_Data.axial_crystals_per_block = value;
}

void CConcordeMainHeader::setIntrinsicCrystalOffset(const int value)
{
	m_Data.intrinsic_crystal_offset = value;
}

void CConcordeMainHeader::setAxialBlocks(const int value)
{
	m_Data.axial_blocks = value;
}

void CConcordeMainHeader::setAxialCrystalPitch(const float value)
{
	m_Data.axial_crystal_pitch = value;
}

void CConcordeMainHeader::setRadius(const float value)
{
	m_Data.radius = value;
}

void CConcordeMainHeader::setRadialFov(const float value)
{
	m_Data.radial_fov = value;
}

void CConcordeMainHeader::setSrcRadius(const float value)
{
	m_Data.src_radius = value;
}

void CConcordeMainHeader::setSrcCmPerRev(const float value)
{
	m_Data.src_cm_per_rev = value;
}

void CConcordeMainHeader::setTxSrcType(const int value)
{
	m_Data.tx_src_type = value;
}

void CConcordeMainHeader::setTransaxialBinSize(const float value)
{
	m_Data.transaxial_bin_size = value;
}

void CConcordeMainHeader::setAxialPlaneSize(const float value)
{
	m_Data.axial_plane_size = value;
}

void CConcordeMainHeader::setLld(const float value)
{
	m_Data.lld = value;
}

void CConcordeMainHeader::setUld(const float value)
{
	m_Data.uld = value;
}

void CConcordeMainHeader::setDataType(const int value)
{
	 m_Data.data_type = value;
}

void CConcordeMainHeader::setDataOrder(const int value){ m_Data.data_order = value;
}

void CConcordeMainHeader::setSpan(const int value)
{
	 m_Data.span = value;
}

void CConcordeMainHeader::setRingDifference(const int value)
{
	 m_Data.ring_difference = value;
}

void CConcordeMainHeader::setNumberOfDimensions(const int value)
{
	 m_Data.number_of_dimensions = value;
}

void CConcordeMainHeader::setXDimension(const int value)
{
	 m_Data.x_dimension = value;
}

void CConcordeMainHeader::setYDimension(const int value)
{
	 m_Data.y_dimension = value;
}

void CConcordeMainHeader::setZDimension(const int value)
{
	 m_Data.z_dimension = value;
}

void CConcordeMainHeader::setWDimension(const int value)
{
	 m_Data.w_dimension = value;
}

void CConcordeMainHeader::setDeltaElements(const int i, const int value)
{
	 m_Data.delta_elements[i] = value;
}

void CConcordeMainHeader::setDeadtimeCorrectionApplied(const int value)
{
	 m_Data.deadtime_correction_applied = value;
}

void CConcordeMainHeader::setDecayCorrectionApplied(const int value)
{
	 m_Data.decay_correction_applied = value;
}

void CConcordeMainHeader::setNormalizationApplied(const int value)
{
	 m_Data.normalization_applied = value;
}

void CConcordeMainHeader::setAttenuationApplied(const int value)
{
	 m_Data.attenuation_applied = value;
}

void CConcordeMainHeader::setScatterCorrection(const int value)
{
	 m_Data.scatter_correction = value;
}

void CConcordeMainHeader::setArcCorrectionApplied(const int value)
{
	 m_Data.arc_correction_applied = value;
}

void CConcordeMainHeader::setPixelSize(const float value)
{
	m_Data.pixel_size = value;
}


void CConcordeMainHeader::setCalibrationUnits(const int value)
{
	 m_Data.calibration_factor = value;
}

void CConcordeMainHeader::setCalibrationFactor(const float value)
{
	 m_Data.calibration_factor = value;
}

void CConcordeMainHeader::setCalibrationBranchingFraction(const float value)
{
	 m_Data.calibration_branching_fraction = value;
}

void CConcordeMainHeader::setNumberOfSinglesRates(const int value)
{
	 m_Data.number_of_singles_rates = value;
}

void CConcordeMainHeader::setInvestigatorName(const QString value)
{
	 m_Data.investigator = value;
}

void CConcordeMainHeader::setOperatorName(const QString value)
{
	 m_Data.Operator = value;
}

void CConcordeMainHeader::setStudyIdentifier(const QString value)
{
	 m_Data.study_identifier = value;
}

void CConcordeMainHeader::setScanTime(const long value)
{
	 m_Data.scan_time.setTime_t(value);
}

void CConcordeMainHeader::setInjectedCompound(const QString value)
{
	 m_Data.injected_compound = value;
}

void CConcordeMainHeader::setDoseUnits(const int value)
{
	 m_Data.dose_units = value;
}

void CConcordeMainHeader::setDose(const float value)
{
	 m_Data.dose = value;
}

void CConcordeMainHeader::setInjectionTime(const long value)
{
	 m_Data.injection_time.setTime_t(value);
}

void CConcordeMainHeader::setInjectionDecayCorrection(const float value)
{
	 m_Data.injection_decay_correction = value;
}

void CConcordeMainHeader::setSubjectIdentifier(const QString value)
{
	 m_Data.subject_identifier = value;
}

void CConcordeMainHeader::setSubjectGenus(const QString value)
{
	 m_Data.subject_genus = value;
}

void CConcordeMainHeader::setSubjectOrientation(const int value)
{
	 m_Data.subject_orientation = value;
}

void CConcordeMainHeader::setSubjectLength_units(const int value)
{
	 m_Data.subject_length_units = value;
}

void CConcordeMainHeader::setSubjectLength(const float value)
{
	 m_Data.subject_length = value;
}

void CConcordeMainHeader::setSubjectWeight_units(const int value)
{
	 m_Data.subject_weight_units = value;
}

void CConcordeMainHeader::setSubjectWeight(const float value)
{
	 m_Data.subject_weight = value;
}

void CConcordeMainHeader::setSubjectPhenotype(const QString value)
{
	 m_Data.subject_phenotype = value;
}

void CConcordeMainHeader::setStudyModel(const QString value)
{
	 m_Data.study_model = value;
}

void CConcordeMainHeader::setAnesthesia(const QString value)
{
	 m_Data.anesthesia = value;
}

void CConcordeMainHeader::setAnalgesia(const QString value)
{
	 m_Data.analgesia = value;
}

void CConcordeMainHeader::setOtherDrugs(const QString value)
{
	 m_Data.other_drugs = value;
}

void CConcordeMainHeader::setFoodAccess(const QString value)
{
	 m_Data.food_access = value;
}

void CConcordeMainHeader::setWaterAccess(const QString value)
{
	 m_Data.water_access = value;
}


//  Class: CConcordeMainHeader
//  Method: setDefaults
//!
//! This methods sets some default values in the header. That minimizes 
//! the occurance of errors due to undefined values.
//!
////////////////////////////////////////////////////////////////////////////////
void CConcordeMainHeader::setDefaults()
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

bool CConcordeMainHeader::load()
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

//  Class: CConcordeMainHeader
//  Method: load
//!
//! loades the headerinformation.
//!
//! @return true on success otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CConcordeMainHeader::load(QString File)
{
	ENTER();
	D("Start Parsing File %s", File.ascii());
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
		D("Headerfile %s not found !", File.ascii());
		return false;
	}
	RETURN(true);
	return true;
}

//  Class: CConcordeMainHeader
//  Method: save
//!
//! Saves the headerinformation.
//!
//! @return true on success otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CConcordeMainHeader::save() const
{
	return true;
}

CMedIOHeader::Format CConcordeMainHeader::headerFormat() const
{
	return CMedIOHeader::ConcordeMicroPetMainHeader;
}

//int CConcordeMainHeader::rtti()
//{
//	return CMedIOHeader::ConcordeMicropet;
//}

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

//  Class: CConcordeMainHeader
//  Method: init
//!
//! //initialise all keys which should be searched for in header file
//!
//! @return true on success otherwise false
////////////////////////////////////////////////////////////////////////////////
bool CConcordeMainHeader::init()
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
	Parser.addKey("isotope_half_life", &m_Data.isotope_half_time);

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
	
	Parser.addKey("calibration_units", &m_Data.calibration_units);
	Parser.addKey("calibration_factor", &m_Data.calibration_factor);
	Parser.addKey("calibration_branching_fraction", &m_Data.calibration_branching_fraction);
	Parser.addKey("number_of_singles_rates", &m_Data.number_of_singles_rates);
	
	Parser.addKey("investigator", &m_Data.investigator);
	Parser.addKey("operator", &m_Data.Operator);
	Parser.addKey("study_identifier", &m_Data.study_identifier);
	Parser.addKey("scan_time", &m_Data.scan_time, CDate::wrapper_parseKeyValue);
	Parser.addKey("injected_compound", &m_Data.injected_compound);
	Parser.addKey("dose_units", &m_Data.dose_units);
	Parser.addKey("dose", &m_Data.dose);
	Parser.addKey("injection_time", &m_Data.injection_time, CDate::wrapper_parseKeyValue);
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

bool CConcordeMainHeader::convertFrom(const CMedIOHeader& srcMainHeader, const CMedIOHeader& srcSubHeader)
{
	copyData(srcMainHeader);
	return true;
}

CMedIOHeader& CConcordeMainHeader::copyData(const CMedIOHeader& src)
{
	ENTER();
	if(src.headerFormat() == CMedIOHeader::ConcordeMicroPetMainHeader)
	{
		//TODO: add CHeaderConcordeFrame to CMedIOHeader and check for it
		W("TODO: add CHeaderConcordeFrame to CMedIOHeader and check for it");
		CConcordeMainHeader* head = (CConcordeMainHeader*)&src;
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
		m_Data.investigator = head->investigatorName();
		m_Data.Operator = head->operatorName();
		m_Data.study_identifier = head->studyIdentifier();
		m_Data.scan_time.setTime_t(head->scanTime());
		m_Data.injected_compound = head->injectedCompound();
		m_Data.dose_units = head->doseUnits();
		m_Data.dose = head->dose();
		m_Data.injection_time.setTime_t(head->injectionTime());
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
