/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004 by Jens Langner <Jens.Langner@light-speed.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

//! @file CConcordeMainHeader.cpp
//! @brief contains the implementation of the class ConcordeMainHeader
//! @author Hagen Moelle
//! @date 09/10/2005

#include <QString>
#include <QList>
#include <QDateTime>

#include "CConcordeMainHeader.h"
#include "CConcordeFile.h"
#include "CMedIOHeader.h"
#include "CMedIOData.h"
#include "CKeyParser.h"
#include "CIntVector.h"
#include "CDate.h"
#include "MedIOUnits.h"

#include <rtdebug.h>

// we define the private inline class of that one so that we
// are able to hide the private methods & data of that class in the
// public headers
class CConcordeMainHeaderPrivate
{
	public:
		void init();
		CKeyParser parser;
		struct HeaderData
		{
			int					model;			//System_Type;
			QString			institution;		//Facility_Name;
			QString			study;			//Study type/description (string)
			QString			file_name;		//Original_File_Name;
			int					file_type;		//File_Type;
			int					acquisition_mode;	//Acquisition_Type;
			int					bed_motion;		//0 static/unknown : 1 continuous motion
			int					total_frames;
			QString			isotope;		//Isotope_Name;
			float				isotope_half_time; 	//Isotope_Halflife;
			float				isotope_branching_fraction;	//Branching_Fraction;
			int					transaxial_crystals_per_block;
			int					axial_crystals_per_block;
			int					intrinsic_crystal_offset;
			int					transaxial_blocks;
			int					axial_blocks;
			float				transaxial_crystal_pitch;
			float				axial_crystal_pitch;
			float				radius;
			float				radial_fov;		//Distance_Scanned Transaxial_FOV;
			float				src_radius;
			float				src_cm_per_rev;
			int					tx_src_type;
			float				transaxial_bin_size;	//Bin_Size; X_Resolution;
			float				axial_plane_size;	//Z_Resolution;
			float				lld;			//Lwr_True_Thres;
			float				uld;			//Upr_True_Thres;
			int					data_type;		//Data_Type;
			int					data_order;		//0 Segment|Axis|View|Ring_Diff
															//1 Segment|View|Axis|Ring_Diff
			int					span;			//Axial_Compression;
			int					ring_difference;	//Ring_Difference;
			int					number_of_dimensions;	//Num_Dimensions;
			int					x_dimension;		//Num_R_Elements;
			int					y_dimension;		//Num_Angles;
			int					z_dimension;
			int					w_dimension;
			CIntVector	delta_elements;	//Num_Z_Elements[64];
			int					deadtime_correction_applied;
			int					decay_correction_applied;
			int					normalization_applied;
			int					attenuation_applied;
			int					scatter_correction;
			int					arc_correction_applied;		//0 none : != 0 true
			float				x_offset;
			float				y_offset;
			float				zoom;
			float				pixel_size;
			int					calibration_units;
			float				calibration_factor;
			float				calibration_branching_fraction;
			int					number_of_singles_rates;
			QString			investigator;
			QString			Operator;
			QString			study_identifier;
			CDate				scan_time;		//Scan_Start_Time;
			QString			injected_compound;
			int					dose_units;
			float				dose;
			CDate				injection_time;
			float				injection_decay_correction;
			QString			subject_identifier;
			QString			subject_genus;
			int					subject_orientation;	//Patient_Orientation;
			int					subject_length_units;
			float				subject_length;
			int					subject_weight_units;
			float				subject_weight;
			QString			subject_phenotype;
			QString			study_model;
			QString			anesthesia;
			QString			analgesia;
			QString			other_drugs;
			QString			food_access;
			QString			water_access;
		} header;
};

//  Class: CConcordeMainHeader
//  Constructor: CConcordeMainHeader
//!
//! constructs a CConcordeMainHeader object
//!
//! @param file: complete path to file holding header
////////////////////////////////////////////////////////////////////////////////
CConcordeMainHeader::CConcordeMainHeader(const QString& fileName)
	: CMedIOHeader(NULL)
{ 
	ENTER();

	// allocate data from our private instance class
	m_pData = new CConcordeMainHeaderPrivate();
	m_pData->init();

	clear();

	if(!load(fileName))
	{
		E("Something is wrong with the headerfile");
	}
	else
		D("Everything ok");

	LEAVE();
}

//  Class: CConcordeMainHeader
//  Constructor: CConcordeMainHeader
//!
//! constructs a CHeaderConcorde object
//!
////////////////////////////////////////////////////////////////////////////////
CConcordeMainHeader::CConcordeMainHeader(CConcordeFile* file)
	: CMedIOHeader(file)
{
	ENTER();
	
	D("Initialising all keyvalues");
	// allocate data from our private instance class
	m_pData = new CConcordeMainHeaderPrivate();
	m_pData->init();
	
	D("Clearing header");
	clear();

	//D("Trying to load headerfile");
	//if(!this->load())
	//{
	//	D("Something is wrong with the headerfile");
	//}
	//else
	//	D("Everything ok");

	LEAVE();
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

	delete m_pData;

	LEAVE();
}

CConcordeMainHeader::CConcordeMainHeader(const CConcordeMainHeader& src)
	: CMedIOHeader(src)
{
	ENTER();

	// allocate data from our private instance class
	m_pData = new CConcordeMainHeaderPrivate(*(src.m_pData));

	LEAVE();
}

CConcordeMainHeader& CConcordeMainHeader::operator=(const CConcordeMainHeader& src)
{
	ENTER();

	m_pData->header.model = src.model();
	m_pData->header.institution = src.institution();
	m_pData->header.study = src.study();
	m_pData->header.file_name = src.fileName();
	m_pData->header.file_type = src.fileType();
	m_pData->header.acquisition_mode = src.acquisitionMode();
	m_pData->header.bed_motion = src.bedMotion();
	m_pData->header.total_frames = src.totalFrames();
	m_pData->header.isotope = src.isotope();
	m_pData->header.isotope_half_time = src.isotopeHalfTime();
	m_pData->header.isotope_branching_fraction = src.isotopeBranchingFraction();
	m_pData->header.transaxial_crystals_per_block = src.transaxialCrystalsPerBlock();
	m_pData->header.axial_crystals_per_block = src.axialCrystalsPerBlock();
	m_pData->header.intrinsic_crystal_offset = src.intrinsicCrystalOffset();
	m_pData->header.axial_blocks = src.axialBlocks();
	m_pData->header.axial_crystal_pitch = src.axialCrystalPitch();
	m_pData->header.radius = src.radius();
	m_pData->header.radial_fov = src.radialFov();
	m_pData->header.src_radius = src.srcRadius();
	m_pData->header.src_cm_per_rev = src.srcCmPerRev();
	m_pData->header.tx_src_type = src.txSrcType();
	m_pData->header.transaxial_bin_size = src.transaxialBinSize();
	m_pData->header.axial_plane_size = src.axialPlaneSize();
	m_pData->header.pixel_size = src.pixelSize();
	m_pData->header.lld = src.lld();
	m_pData->header.uld = src.uld();
	m_pData->header.data_type = src.dataType();
	m_pData->header.data_order = src.dataOrder();
	m_pData->header.span = src.span();
	m_pData->header.ring_difference = src.ringDifference();
	m_pData->header.number_of_dimensions = src.numberOfDimensions();
	m_pData->header.x_dimension = src.xDimension();
	m_pData->header.y_dimension = src.yDimension();
	m_pData->header.z_dimension = src.zDimension();
	m_pData->header.w_dimension = src.wDimension();
	
	if(m_pData->header.w_dimension > 1)
		for(int i = 0; i < m_pData->header.w_dimension; i++)
			m_pData->header.delta_elements[i] = src.deltaElements(i);

	m_pData->header.deadtime_correction_applied = src.deadtimeCorrectionApplied();
	m_pData->header.decay_correction_applied = src.decayCorrectionApplied();
	m_pData->header.normalization_applied = src.normalizationApplied();
	m_pData->header.attenuation_applied = src.attenuationApplied();
	m_pData->header.scatter_correction = src.scatterCorrection();
	m_pData->header.arc_correction_applied = src.arcCorrectionApplied();
	m_pData->header.calibration_factor = src.calibrationFactor();
	m_pData->header.calibration_branching_fraction = src.calibrationBranchingFraction();
	m_pData->header.calibration_units = src.calibrationUnits();
	m_pData->header.number_of_singles_rates = src.numberOfSinglesRates();
	m_pData->header.investigator = src.investigatorName();
	m_pData->header.Operator = src.operatorName();
	m_pData->header.study_identifier = src.studyIdentifier();
	m_pData->header.scan_time.setTime_t(src.scanTime());
	m_pData->header.injected_compound = src.injectedCompound();
	m_pData->header.dose_units = src.doseUnits();
	m_pData->header.dose = src.dose();
	m_pData->header.injection_time.setTime_t(src.injectionTime());
	m_pData->header.injection_decay_correction = src.injectionDecayCorrection();
	m_pData->header.subject_identifier = src.subjectIdentifier();
	m_pData->header.subject_genus = src.subjectGenus();
	m_pData->header.subject_orientation = src.subjectOrientation();
	m_pData->header.subject_length_units = src.subjectLengthUnits();
	m_pData->header.subject_length = src.subjectLength();
	m_pData->header.subject_weight_units = src.subjectWeightUnits();
	m_pData->header.subject_weight = src.subjectWeight();
	m_pData->header.subject_phenotype = src.subjectPhenotype();
	m_pData->header.study_model = src.studyModel();
	m_pData->header.anesthesia = src.anesthesia();
	m_pData->header.analgesia = src.analgesia();
	m_pData->header.other_drugs = src.otherDrugs();
	m_pData->header.food_access = src.foodAccess();
	m_pData->header.water_access = src.waterAccess();

	LEAVE();
	return *this;
}

int	CConcordeMainHeader::model(void) const
{
	return m_pData->header.model;
}

QString	CConcordeMainHeader::institution(void) const
{
	return m_pData->header.institution;
}

QString	CConcordeMainHeader::study(void) const
{
	return m_pData->header.study;
}

QString	CConcordeMainHeader::fileName(void) const
{
	return m_pData->header.file_name;
}

CConcordeMainHeader::FileType CConcordeMainHeader::fileType(void) const
{
	return static_cast<FileType>(m_pData->header.file_type);
}

CConcordeMainHeader::AquisitionMode CConcordeMainHeader::acquisitionMode(void) const
{
	return static_cast<AquisitionMode>(m_pData->header.acquisition_mode);
}

CConcordeMainHeader::BedMotion CConcordeMainHeader::bedMotion(void) const
{
	return static_cast<BedMotion>(m_pData->header.bed_motion);
}

int CConcordeMainHeader::totalFrames(void) const
{
	return m_pData->header.total_frames;
}

QString CConcordeMainHeader::isotope(void) const
{
	return m_pData->header.isotope;
}

float CConcordeMainHeader::isotopeHalfTime(void) const
{
	return m_pData->header.isotope_half_time;
}

float CConcordeMainHeader::isotopeBranchingFraction(void) const
{
	return m_pData->header.isotope_branching_fraction;
}

int CConcordeMainHeader::transaxialCrystalsPerBlock(void) const
{
	return m_pData->header.transaxial_crystals_per_block;
}

int CConcordeMainHeader::axialCrystalsPerBlock(void) const
{
	return m_pData->header.axial_crystals_per_block;
}

int CConcordeMainHeader::intrinsicCrystalOffset(void) const
{
	return m_pData->header.intrinsic_crystal_offset;
}

int CConcordeMainHeader::axialBlocks(void) const
{
	return m_pData->header.axial_blocks;
}

float CConcordeMainHeader::axialCrystalPitch(void) const
{
	return m_pData->header.axial_crystal_pitch;
}

float CConcordeMainHeader::radius(void) const
{
	return m_pData->header.radius;
}

float CConcordeMainHeader::radialFov(void) const
{
	return m_pData->header.radial_fov;
}

float CConcordeMainHeader::srcRadius(void) const
{
	return m_pData->header.src_radius;
}

float CConcordeMainHeader::srcCmPerRev(void) const
{
	return m_pData->header.src_cm_per_rev;
}

CConcordeMainHeader::SourceType CConcordeMainHeader::txSrcType(void) const
{
	return static_cast<SourceType>(m_pData->header.tx_src_type);
}

float CConcordeMainHeader::transaxialBinSize(void) const
{
	return m_pData->header.transaxial_bin_size;
}

float CConcordeMainHeader::axialPlaneSize(void) const
{
	return m_pData->header.axial_plane_size;
}

float CConcordeMainHeader::lld(void) const
{
	return m_pData->header.lld;
}

float CConcordeMainHeader::uld(void) const
{
	return m_pData->header.uld;
}

CConcordeMainHeader::DataType CConcordeMainHeader::dataType(void) const
{
	return static_cast<DataType>(m_pData->header.data_type);
}

CConcordeMainHeader::DataOrder CConcordeMainHeader::dataOrder(void) const
{
	return static_cast<DataOrder>(m_pData->header.data_order);
}

int CConcordeMainHeader::span(void) const
{
	return m_pData->header.span;
}

int CConcordeMainHeader::ringDifference(void) const
{
	return m_pData->header.ring_difference;
}
		
int CConcordeMainHeader::numberOfDimensions(void) const
{
	return m_pData->header.number_of_dimensions;
}

int CConcordeMainHeader::xDimension(void) const
{
	return m_pData->header.x_dimension;
}

int CConcordeMainHeader::yDimension(void) const
{
	return m_pData->header.y_dimension;
}

int CConcordeMainHeader::zDimension(void) const
{
	return m_pData->header.z_dimension;
}

int CConcordeMainHeader::wDimension(void) const
{
	return m_pData->header.w_dimension;
}

int CConcordeMainHeader::deltaElements(int i) const
{
	return m_pData->header.delta_elements[i];
}

CConcordeMainHeader::DeadTimeCorrectionApplied CConcordeMainHeader::deadtimeCorrectionApplied(void) const
{
	return static_cast<DeadTimeCorrectionApplied>(m_pData->header.deadtime_correction_applied);
}

CConcordeMainHeader::DecayCorrectionApplied CConcordeMainHeader::decayCorrectionApplied(void) const
{
	return static_cast<DecayCorrectionApplied>(m_pData->header.decay_correction_applied);
}

CConcordeMainHeader::NormalizationApplied CConcordeMainHeader::normalizationApplied(void) const
{
	return static_cast<NormalizationApplied>(m_pData->header.normalization_applied);
}

CConcordeMainHeader::AttenuationApplied CConcordeMainHeader::attenuationApplied(void) const
{
	return static_cast<AttenuationApplied>(m_pData->header.attenuation_applied);
}

CConcordeMainHeader::ScatterCorrection CConcordeMainHeader::scatterCorrection(void) const
{
	return static_cast<ScatterCorrection>(m_pData->header.scatter_correction);
}

CConcordeMainHeader::ArcCorrectionApplied CConcordeMainHeader::arcCorrectionApplied(void) const
{
	return static_cast<ArcCorrectionApplied>(m_pData->header.arc_correction_applied);
}

float CConcordeMainHeader::pixelSize(void) const
{
	return m_pData->header.pixel_size;
}

CConcordeMainHeader::CalibrationUnits CConcordeMainHeader::calibrationUnits(void) const
{
	return static_cast<CalibrationUnits>(m_pData->header.calibration_units);
}

float CConcordeMainHeader::calibrationFactor(void) const
{
	return m_pData->header.calibration_factor;
}

float CConcordeMainHeader::calibrationBranchingFraction(void) const
{
	return m_pData->header.calibration_branching_fraction;
}

int CConcordeMainHeader::numberOfSinglesRates(void) const
{
	return m_pData->header.number_of_singles_rates;
}

QString CConcordeMainHeader::investigatorName(void) const
{
	return m_pData->header.investigator;
}

QString CConcordeMainHeader::operatorName(void) const
{
	return m_pData->header.Operator;
}

QString CConcordeMainHeader::studyIdentifier(void) const
{
	return m_pData->header.study_identifier;
}

long CConcordeMainHeader::scanTime(void) const
{
	return m_pData->header.scan_time.toTime_t();
}

QString CConcordeMainHeader::injectedCompound(void) const
{
	return m_pData->header.injected_compound;
}

CConcordeMainHeader::DoseUnits CConcordeMainHeader::doseUnits(void) const
{
	return static_cast<DoseUnits>(m_pData->header.dose_units);
}

float CConcordeMainHeader::dose(void) const
{
	return m_pData->header.dose;
}

long CConcordeMainHeader::injectionTime(void) const
{
	return m_pData->header.injection_time.toTime_t();
}

float CConcordeMainHeader::injectionDecayCorrection(void) const
{
	return m_pData->header.injection_decay_correction;
}

QString CConcordeMainHeader::subjectIdentifier(void) const
{
	return m_pData->header.subject_identifier;
}

QString CConcordeMainHeader::subjectGenus(void) const
{
	return m_pData->header.subject_genus;
}

CConcordeMainHeader::SubjectOrientation CConcordeMainHeader::subjectOrientation(void) const
{
	return static_cast<SubjectOrientation>(m_pData->header.subject_orientation);
}

CConcordeMainHeader::SubjectLengthUnits CConcordeMainHeader::subjectLengthUnits(void) const
{
	return static_cast<SubjectLengthUnits>(m_pData->header.subject_length_units);
}

float CConcordeMainHeader::subjectLength(void) const
{
	return m_pData->header.subject_length;
}

CConcordeMainHeader::SubjectWeightUnits CConcordeMainHeader::subjectWeightUnits(void) const
{
	return static_cast<SubjectWeightUnits>(m_pData->header.subject_weight_units);
}

float CConcordeMainHeader::subjectWeight(void) const
{
	return m_pData->header.subject_weight;
}

QString CConcordeMainHeader::subjectPhenotype(void) const
{
	return m_pData->header.subject_phenotype;
}

QString CConcordeMainHeader::studyModel(void) const
{
	return m_pData->header.study_model;
}

QString CConcordeMainHeader::anesthesia(void) const
{
	return m_pData->header.anesthesia;
}

QString CConcordeMainHeader::analgesia(void) const
{
	return m_pData->header.analgesia;
}

QString CConcordeMainHeader::otherDrugs(void) const
{
	return m_pData->header.other_drugs;
}

QString CConcordeMainHeader::foodAccess(void) const
{
	return m_pData->header.food_access;
}

QString CConcordeMainHeader::waterAccess(void) const
{
	return m_pData->header.water_access;
}

QDateTime CConcordeMainHeader::scanTimeQt(void) const
{
	return static_cast<QDateTime>(m_pData->header.scan_time);
}

QDateTime CConcordeMainHeader::injectionTimeQt(void) const
{
	return static_cast<QDateTime>(m_pData->header.injection_time);
}

QString CConcordeMainHeader::strSubjectWeightUnits(void) const
{
	CWeight weight;
	switch(m_pData->header.subject_weight_units)
	{
		case Grams: weight.setUnit(CWeight::Gram_units); break;
		case Ounces: weight.setUnit(CWeight::Ounce_units); break;
		case Kilograms: weight.setUnit(CWeight::Kilogram_units); break;
		case Pounds: weight.setUnit(CWeight::Pound_units); break;
		default: weight.setUnit(CWeight::Unknown_units); break;
	}
	return weight.unitAsString();
}

QString CConcordeMainHeader::strSubjectLengthUnits(void) const
{
	CLength length; 
	switch(m_pData->header.subject_length_units)
	{
		case Millimeters: length.setUnit(CLength::Millimeter_units); break;
		case Centimeters: length.setUnit(CLength::Centimeter_units); break;
		case Inches: length.setUnit(CLength::Inch_units); break;
		default: length.setUnit(CLength::Unknown_units); break;
	}
	return length.unitAsString();
}

QString CConcordeMainHeader::strDoseUnits(void) const
{
	CDosage dosage;
	switch(m_pData->header.dose_units)
	{
		case mCi: dosage.setUnit(CDosage::Millicurie_units); break;
		case MBq: dosage.setUnit(CDosage::Megabequerels_units); break;
		default: dosage.setUnit(CDosage::Unknown_units); break;
	}
	return dosage.unitAsString();
}

//mutator methods
void CConcordeMainHeader::setModel(const int value)
{
	 m_pData->header.model = value;
}

void CConcordeMainHeader::setInstitution(const QString value)
{
	 m_pData->header.institution = value;
}

void CConcordeMainHeader::setFileName(const QString value)
{
	 m_pData->header.file_name = value;
}

void CConcordeMainHeader::setFileType(const int value)
{
	 m_pData->header.file_type = value;
}

void CConcordeMainHeader::setAcquisitionMode(const int value)
{
	 m_pData->header.acquisition_mode = value;
}

void CConcordeMainHeader::setBedMotion(const int value)
{
	 m_pData->header.bed_motion = value;
}

void CConcordeMainHeader::setNumFrames(const int value)
{
	 m_pData->header.total_frames = value;
}

void CConcordeMainHeader::setIsotope(const QString value)
{
	 m_pData->header.isotope = value;
}

void CConcordeMainHeader::setIsotopeHalftime(const float value)
{
	 m_pData->header.isotope_half_time = value;
}

void CConcordeMainHeader::setIsotopeBranchingFraction(const float value)
{
	 m_pData->header.isotope_branching_fraction = value;
}

void CConcordeMainHeader::setTransaxialCrystalsPerBlock(const int value)
{
	m_pData->header.transaxial_crystals_per_block = value;
}

void CConcordeMainHeader::setAxialCrystalsPerBlock(const int value)
{
	m_pData->header.axial_crystals_per_block = value;
}

void CConcordeMainHeader::setIntrinsicCrystalOffset(const int value)
{
	m_pData->header.intrinsic_crystal_offset = value;
}

void CConcordeMainHeader::setAxialBlocks(const int value)
{
	m_pData->header.axial_blocks = value;
}

void CConcordeMainHeader::setAxialCrystalPitch(const float value)
{
	m_pData->header.axial_crystal_pitch = value;
}

void CConcordeMainHeader::setRadius(const float value)
{
	m_pData->header.radius = value;
}

void CConcordeMainHeader::setRadialFov(const float value)
{
	m_pData->header.radial_fov = value;
}

void CConcordeMainHeader::setSrcRadius(const float value)
{
	m_pData->header.src_radius = value;
}

void CConcordeMainHeader::setSrcCmPerRev(const float value)
{
	m_pData->header.src_cm_per_rev = value;
}

void CConcordeMainHeader::setTxSrcType(const int value)
{
	m_pData->header.tx_src_type = value;
}

void CConcordeMainHeader::setTransaxialBinSize(const float value)
{
	m_pData->header.transaxial_bin_size = value;
}

void CConcordeMainHeader::setAxialPlaneSize(const float value)
{
	m_pData->header.axial_plane_size = value;
}

void CConcordeMainHeader::setLld(const float value)
{
	m_pData->header.lld = value;
}

void CConcordeMainHeader::setUld(const float value)
{
	m_pData->header.uld = value;
}

void CConcordeMainHeader::setDataType(const int value)
{
	 m_pData->header.data_type = value;
}

void CConcordeMainHeader::setDataOrder(const int value){ m_pData->header.data_order = value;
}

void CConcordeMainHeader::setSpan(const int value)
{
	 m_pData->header.span = value;
}

void CConcordeMainHeader::setRingDifference(const int value)
{
	 m_pData->header.ring_difference = value;
}

void CConcordeMainHeader::setNumberOfDimensions(const int value)
{
	 m_pData->header.number_of_dimensions = value;
}

void CConcordeMainHeader::setXDimension(const int value)
{
	 m_pData->header.x_dimension = value;
}

void CConcordeMainHeader::setYDimension(const int value)
{
	 m_pData->header.y_dimension = value;
}

void CConcordeMainHeader::setZDimension(const int value)
{
	 m_pData->header.z_dimension = value;
}

void CConcordeMainHeader::setWDimension(const int value)
{
	 m_pData->header.w_dimension = value;
}

void CConcordeMainHeader::setDeltaElements(const int i, const int value)
{
	 m_pData->header.delta_elements[i] = value;
}

void CConcordeMainHeader::setDeadtimeCorrectionApplied(const int value)
{
	 m_pData->header.deadtime_correction_applied = value;
}

void CConcordeMainHeader::setDecayCorrectionApplied(const int value)
{
	 m_pData->header.decay_correction_applied = value;
}

void CConcordeMainHeader::setNormalizationApplied(const int value)
{
	 m_pData->header.normalization_applied = value;
}

void CConcordeMainHeader::setAttenuationApplied(const int value)
{
	 m_pData->header.attenuation_applied = value;
}

void CConcordeMainHeader::setScatterCorrection(const int value)
{
	 m_pData->header.scatter_correction = value;
}

void CConcordeMainHeader::setArcCorrectionApplied(const int value)
{
	 m_pData->header.arc_correction_applied = value;
}

void CConcordeMainHeader::setPixelSize(const float value)
{
	m_pData->header.pixel_size = value;
}


void CConcordeMainHeader::setCalibrationUnits(const int value)
{
	 m_pData->header.calibration_factor = value;
}

void CConcordeMainHeader::setCalibrationFactor(const float value)
{
	 m_pData->header.calibration_factor = value;
}

void CConcordeMainHeader::setCalibrationBranchingFraction(const float value)
{
	 m_pData->header.calibration_branching_fraction = value;
}

void CConcordeMainHeader::setNumberOfSinglesRates(const int value)
{
	 m_pData->header.number_of_singles_rates = value;
}

void CConcordeMainHeader::setInvestigatorName(const QString value)
{
	 m_pData->header.investigator = value;
}

void CConcordeMainHeader::setOperatorName(const QString value)
{
	 m_pData->header.Operator = value;
}

void CConcordeMainHeader::setStudyIdentifier(const QString value)
{
	 m_pData->header.study_identifier = value;
}

void CConcordeMainHeader::setScanTime(const long value)
{
	 m_pData->header.scan_time.setTime_t(value);
}

void CConcordeMainHeader::setInjectedCompound(const QString value)
{
	 m_pData->header.injected_compound = value;
}

void CConcordeMainHeader::setDoseUnits(const int value)
{
	 m_pData->header.dose_units = value;
}

void CConcordeMainHeader::setDose(const float value)
{
	 m_pData->header.dose = value;
}

void CConcordeMainHeader::setInjectionTime(const long value)
{
	 m_pData->header.injection_time.setTime_t(value);
}

void CConcordeMainHeader::setInjectionDecayCorrection(const float value)
{
	 m_pData->header.injection_decay_correction = value;
}

void CConcordeMainHeader::setSubjectIdentifier(const QString value)
{
	 m_pData->header.subject_identifier = value;
}

void CConcordeMainHeader::setSubjectGenus(const QString value)
{
	 m_pData->header.subject_genus = value;
}

void CConcordeMainHeader::setSubjectOrientation(const int value)
{
	 m_pData->header.subject_orientation = value;
}

void CConcordeMainHeader::setSubjectLength_units(const int value)
{
	 m_pData->header.subject_length_units = value;
}

void CConcordeMainHeader::setSubjectLength(const float value)
{
	 m_pData->header.subject_length = value;
}

void CConcordeMainHeader::setSubjectWeight_units(const int value)
{
	 m_pData->header.subject_weight_units = value;
}

void CConcordeMainHeader::setSubjectWeight(const float value)
{
	 m_pData->header.subject_weight = value;
}

void CConcordeMainHeader::setSubjectPhenotype(const QString value)
{
	 m_pData->header.subject_phenotype = value;
}

void CConcordeMainHeader::setStudyModel(const QString value)
{
	 m_pData->header.study_model = value;
}

void CConcordeMainHeader::setAnesthesia(const QString value)
{
	 m_pData->header.anesthesia = value;
}

void CConcordeMainHeader::setAnalgesia(const QString value)
{
	 m_pData->header.analgesia = value;
}

void CConcordeMainHeader::setOtherDrugs(const QString value)
{
	 m_pData->header.other_drugs = value;
}

void CConcordeMainHeader::setFoodAccess(const QString value)
{
	 m_pData->header.food_access = value;
}

void CConcordeMainHeader::setWaterAccess(const QString value)
{
	 m_pData->header.water_access = value;
}


//  Class: CConcordeMainHeader
//  Method: setDefaults
//!
//! This methods sets some default values in the header. That minimizes 
//! the occurance of errors due to undefined values.
//!
////////////////////////////////////////////////////////////////////////////////
void CConcordeMainHeader::clear()
{
	m_pData->header.model = 0;
	m_pData->header.institution = QString();
	m_pData->header.study = QString();
	m_pData->header.file_name = QString();
	m_pData->header.file_type = 0;
	m_pData->header.acquisition_mode = 0;
	m_pData->header.bed_motion = 0;
	m_pData->header.total_frames = 0;
	m_pData->header.isotope = QString();
	m_pData->header.isotope_half_time = 0.0F;
	m_pData->header.isotope_branching_fraction = 0.0F;
	m_pData->header.transaxial_crystals_per_block = 0;
	m_pData->header.axial_crystals_per_block = 0;
	m_pData->header.intrinsic_crystal_offset = 0;
	m_pData->header.transaxial_blocks = 0;
	m_pData->header.axial_blocks = 0;
	m_pData->header.transaxial_crystal_pitch = 0.0F;
	m_pData->header.axial_crystal_pitch = 0.0F;
	m_pData->header.radius = 0.0F;
	m_pData->header.radial_fov = 0.0F;
	m_pData->header.src_radius = 0.0F;
	m_pData->header.src_cm_per_rev = 0.0F;
	m_pData->header.tx_src_type = 0;
	m_pData->header.transaxial_bin_size = 0.0F;
	m_pData->header.axial_plane_size = 0.0F;
	m_pData->header.lld = 0.0F;
	m_pData->header.uld = 0.0F;
	m_pData->header.data_type = 0;
	m_pData->header.data_order = 0;

	m_pData->header.span = 0;
	m_pData->header.ring_difference = 0;
	m_pData->header.number_of_dimensions = 0;
	m_pData->header.x_dimension = 0;
	m_pData->header.y_dimension = 0;
	m_pData->header.z_dimension = 0;
	m_pData->header.w_dimension = 0;

	m_pData->header.deadtime_correction_applied = 0;
	m_pData->header.decay_correction_applied = 0;
	m_pData->header.normalization_applied = 0;
	m_pData->header.attenuation_applied = 0;
	m_pData->header.scatter_correction = 0;
	m_pData->header.arc_correction_applied = 0;
	m_pData->header.x_offset = 0.0F;
	m_pData->header.y_offset = 0.0F;
	m_pData->header.zoom = 1.0F;
	m_pData->header.pixel_size = 0.0F;
	m_pData->header.calibration_units = 0;
	m_pData->header.calibration_factor = 0.0F;
	m_pData->header.calibration_branching_fraction = 0.0F;
	m_pData->header.number_of_singles_rates = 0;

	m_pData->header.investigator = QString();
	m_pData->header.Operator = QString();
	m_pData->header.study_identifier = QString();
	m_pData->header.injected_compound = QString();

	m_pData->header.dose_units = 0;
	m_pData->header.dose = 0.0F;
	m_pData->header.injection_decay_correction = 0.0;

	m_pData->header.subject_identifier = QString();
	m_pData->header.subject_genus = QString();

	m_pData->header.subject_orientation = 0;
	m_pData->header.subject_length_units = 0;
	m_pData->header.subject_length = 0.0F;
	m_pData->header.subject_weight_units = 0;
	m_pData->header.subject_weight = 0.0F;

	m_pData->header.subject_phenotype = QString();
	m_pData->header.study_model = QString();
	m_pData->header.anesthesia = QString();
	m_pData->header.analgesia = QString();
	m_pData->header.other_drugs = QString();
	m_pData->header.food_access = QString();
	m_pData->header.water_access = QString();
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
bool CConcordeMainHeader::load(const QString& File)
{
	ENTER();
	D("Start Parsing File %s", File.toAscii().data());
	if(m_pData->parser.parse(File))
	{
		D("Parsing of mainheader seems to be ok");
		//for(int i = 0; i < m_pData->header.total_frames; i++)
		//{
		//	CHeaderConcordeFrame* frame = new CHeaderConcordeFrame(File, i);
		//	frames.append(frame);
		//}
		
	}
	else
	{
		D("Headerfile %s not found !", File.toAscii().data());
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

//  Class: CConcordeMainHeader
//  Method: init
//!
//! //initialise all keys which should be searched for in header file
//!
////////////////////////////////////////////////////////////////////////////////
void CConcordeMainHeaderPrivate::init()
{
	parser.addSeparator(" ");
	parser.addComment("#");
	parser.addStopSymbol("end_of_header");

	parser.addKey("model", &header.model);
	parser.addKey("institution", &header.institution);
	parser.addKey("study", &header.study);
	parser.addKey("file_name", &header.file_name);
	parser.addKey("file_type", &header.file_type);
	parser.addKey("acquisition_mode", &header.acquisition_mode);
	parser.addKey("bed_motion", &header.bed_motion);
	parser.addKey("total_frames", &header.total_frames);
	
	parser.addKey("isotope", &header.isotope);
	parser.addKey("isotope_half_time", &header.isotope_half_time);
	parser.addKey("isotope_half_life", &header.isotope_half_time);

	parser.addKey("isotope_branching_fraction", &header.isotope_branching_fraction);
	
	parser.addKey("transaxial_crystals_per_block", &header.transaxial_crystals_per_block);
	parser.addKey("axial_crystals_per_block", &header.axial_crystals_per_block);
	parser.addKey("intrinsic_crystal_offset", &header.intrinsic_crystal_offset);
	parser.addKey("axial_blocks", &header.axial_blocks);
	parser.addKey("axial_crystal_pitch", &header.axial_crystal_pitch);
	parser.addKey("radius", &header.radius);
	parser.addKey("radial_fov", &header.radial_fov);
	parser.addKey("src_radius", &header.src_radius);
	parser.addKey("src_cm_per_rev", &header.src_cm_per_rev);
	parser.addKey("tx_src_type", &header.tx_src_type);
	parser.addKey("transaxial_bin_size", &header.transaxial_bin_size);
	parser.addKey("axial_plane_size", &header.axial_plane_size);
	parser.addKey("pixel_size", &header.pixel_size);
	parser.addKey("lld", &header.lld);
	parser.addKey("uld", &header.uld);
	
	parser.addKey("data_type", &header.data_type);
	parser.addKey("data_order", &header.data_order);
	parser.addKey("span", &header.span);
	parser.addKey("ring_difference", &header.ring_difference);
	parser.addKey("number_of_dimensions", &header.number_of_dimensions);
	parser.addKey("x_dimension", &header.x_dimension);
	parser.addKey("y_dimension", &header.y_dimension);
	parser.addKey("z_dimension", &header.z_dimension);
	parser.addKey("w_dimension", &header.w_dimension);
	parser.addKey("delta_elements", &header.delta_elements, CIntVector::wrapper_parseKeyValue);	
	
	parser.addKey("deadtime_correction_applied", &header.deadtime_correction_applied);
	parser.addKey("decay_correction_applied", &header.decay_correction_applied);
	parser.addKey("normalization_applied", &header.normalization_applied);
	parser.addKey("attenuation_applied", &header.attenuation_applied);
	parser.addKey("scatter_correction", &header.scatter_correction);
	parser.addKey("arc_correction", &header.arc_correction_applied);
	
	parser.addKey("calibration_units", &header.calibration_units);
	parser.addKey("calibration_factor", &header.calibration_factor);
	parser.addKey("calibration_branching_fraction", &header.calibration_branching_fraction);
	parser.addKey("number_of_singles_rates", &header.number_of_singles_rates);
	
	parser.addKey("investigator", &header.investigator);
	parser.addKey("operator", &header.Operator);
	parser.addKey("study_identifier", &header.study_identifier);
	parser.addKey("scan_time", &header.scan_time, CDate::wrapper_parseKeyValue);
	parser.addKey("injected_compound", &header.injected_compound);
	parser.addKey("dose_units", &header.dose_units);
	parser.addKey("dose", &header.dose);
	parser.addKey("injection_time", &header.injection_time, CDate::wrapper_parseKeyValue);
	parser.addKey("injection_decay_correction", &header.injection_decay_correction);
	
	parser.addKey("subject_identifier", &header.subject_identifier);
	parser.addKey("subject_genus", &header.subject_genus);
	parser.addKey("subject_orientation", &header.subject_orientation);
	parser.addKey("subject_length_units", &header.subject_length_units);
	parser.addKey("subject_length", &header.subject_length);
	parser.addKey("subject_weight_units", &header.subject_weight_units);
	parser.addKey("subject_weight", &header.subject_weight);
	parser.addKey("subject_phenotype", &header.subject_phenotype);
	parser.addKey("study_model", &header.study_model);
	
	parser.addKey("anesthesia", &header.anesthesia);
	parser.addKey("analgesia", &header.analgesia);
	parser.addKey("other_drugs", &header.other_drugs);
	parser.addKey("food_access", &header.food_access);
	parser.addKey("water_access", &header.water_access);
}

bool CConcordeMainHeader::convertFrom(const CMedIOHeader* srcHeader1, const CMedIOHeader*)
{
	ENTER();
	bool bResult = false;

	if(srcHeader1)
	{
		switch(srcHeader1->headerFormat())
		{
			case CMedIOHeader::ConcordeMicroPetMainHeader:
			{
				const CConcordeMainHeader* head = static_cast<const CConcordeMainHeader*>(srcHeader1);

				// use the assignment operator to copy everything
				*this = *head;
				
				bResult = true;
			}
			break;
			
			default:
			{
				E("File format not supported yet");
				bResult = false;
			}
			break;
		}
	}
	
	RETURN(bResult);
	return bResult;
}
