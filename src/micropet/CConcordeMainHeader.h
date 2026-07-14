/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2026 hzdr.de and contributors
 
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
    http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

//! @file CConcordeMainHeader.h
//! @date 09/10/2005
//! @brief contains definition of the class CConcordeMainHeader

#ifndef CCONCORDEMAINHEADER_H
#define CCONCORDEMAINHEADER_H

#include <qdatetime.h>

#ifndef __MEDIO_PRIVATE__
#include <CMedIOHeader>
#else
#include <CMedIOHeader.h>
#endif

// forward declarations
class CConcordeMainHeaderPrivate;
class CConcordeFile;

class CConcordeMainHeader : public CMedIOHeader
{
  public:
    enum FileType{UnknownFile = 0, ListMode, Sinogram, Normalization, Attenuation,
        Image, Blank, MuMap = 8, Scatter};
    enum AquisitionMode{UnknownAquisition = 0, BlankAquisition, Emission, Dynamic, Gated,
          ContinuousBedMotion, SinglesTransmission,
          WindowedCoincidenceTransmission,
          NonWindowedCoincidenceTransmission};
    enum BedMotion{Static = 0, Continuous};
    enum SourceType{UnknownSource = 0, Point, Line};
    enum DataType{UnknownDataType = 0, Byte, IntelShort, IntelInt, IntelFloat,
        IEEEFloat, SunShort, SunInt};
    enum DataOrder{ViewMode = 0, SinogramMode};
    enum DeadTimeCorrectionApplied{NoDeadTimeCorrection = 0, Global, CMS};
    enum DecayCorrectionApplied{NoDecayCorrection = 0, 
            DecayCorrection = 1};
    enum NormalizationApplied{NoNormalization = 0, PointSourceInversion, 
            PointSourceComponentBased, 
            CylinderSourceInversion,
            CylinderSourceComponentBased};
    enum AttenuationApplied{NoAttenuation = 0, PointSourceWindowedTX, PointSourceSingles,
          SegmentedPointSourceTX, SegmentedPointSourceSingles,
          Geometry, NonPositronSource, PointSourceNonWindowedTX};
    enum ScatterCorrection{NoScatterCorrection = 0, FitOfEmissionTail, MonteCarlo, 
          DirectCalculation, ModelBased, TXOffWindow};
    enum ArcCorrectionApplied{NoArcCorrection = 0, ArcCorrection = 1};
    enum CalibrationUnits{UnknownCalibrationUnits = 0, nCiPerCC, BqPerCC};
    enum DoseUnits{UnknownDoseUnits = 0, mCi, MBq};
    enum SubjectOrientation{UnknownSubjectOrientation = 0, FeetFirstProne, HeadFirstProne,
          FeetFirstSupine, HeadFirstSupine, FeetFirstRight,
          HeadFirstRight, FeetFirstLeft, HeadFirstLeft};
    enum SubjectLengthUnits{UnknownSubjectLengthUnits = 0, Millimeters, Centimeters, Inches};
    enum SubjectWeightUnits{UnknownSubjectWeightUnits = 0, Grams, Ounces, Kilograms, Pounds};
    
    //! @brief constructor
    //! @param File: complete path to file holding header
    CConcordeMainHeader(const QString& fileName);

    //! @brief default constructor
    CConcordeMainHeader(CConcordeFile* file = NULL);

    //! @brief destructor
    ~CConcordeMainHeader();  

    // copy constructur and default assignment operator
    CConcordeMainHeader(const CConcordeMainHeader& src);    
    CConcordeMainHeader& operator=(const CConcordeMainHeader& src);
    
    // header clear method
    void clear();
    
    bool load();
    bool load(const QString& fileName);
    bool save() const;
    CMedIOHeader::Format headerFormat() const;
    
    //! @return framesize of a sinogram in bytes
    virtual unsigned int frameSize() = 0;

    // clone methods
    virtual CMedIOHeader* clone() const = 0;

    int model(void) const;
    QString institution(void) const;
    QString study(void) const;
    QString fileName(void) const;
    FileType fileType(void) const;
    AquisitionMode acquisitionMode(void) const; 
    BedMotion bedMotion(void) const;
    int totalFrames(void) const;
    
    QString isotope(void) const;
    float isotopeHalfTime(void) const;
    float isotopeBranchingFraction(void) const;
    
    int transaxialCrystalsPerBlock(void) const;
    int axialCrystalsPerBlock(void) const;
    int intrinsicCrystalOffset(void) const;
    int axialBlocks(void) const;
    float axialCrystalPitch(void) const;
    float radius(void) const;
    float radialFov(void) const;
    float srcRadius(void) const;
    float srcCmPerRev(void) const;
    SourceType txSrcType(void) const;
    float transaxialBinSize(void) const;
    float axialPlaneSize(void) const;
    float lld(void) const;
    float uld(void) const;
    
    DataType dataType(void) const;
    DataOrder dataOrder(void) const;
    int span(void) const;
    int ringDifference(void) const;
    int numberOfDimensions(void) const;
    int xDimension(void) const;
    int yDimension(void) const;
    int zDimension(void) const;
    int wDimension(void) const;
    int deltaElements(int i) const;
    
    DeadTimeCorrectionApplied deadtimeCorrectionApplied(void) const;
    DecayCorrectionApplied decayCorrectionApplied(void) const;
    NormalizationApplied normalizationApplied(void) const;
    AttenuationApplied attenuationApplied(void) const;
    ScatterCorrection scatterCorrection(void) const;
    ArcCorrectionApplied arcCorrectionApplied(void) const;
    
    float pixelSize(void) const;

    CalibrationUnits calibrationUnits(void) const; 
    float calibrationFactor(void) const;
    float calibrationBranchingFraction(void) const;
    int numberOfSinglesRates(void) const;
    
    QString investigatorName(void) const;
    QString operatorName(void) const;
    QString studyIdentifier(void) const;
    long scanTime(void) const;
    QString injectedCompound(void) const;
    DoseUnits doseUnits(void) const;
    float dose(void) const;
    long injectionTime(void) const;
    float injectionDecayCorrection(void) const;
    
    
    QString subjectIdentifier(void) const;
    QString subjectGenus(void) const;
    SubjectOrientation subjectOrientation(void) const;
    SubjectLengthUnits subjectLengthUnits(void) const;
    float subjectLength(void) const;
    SubjectWeightUnits subjectWeightUnits(void) const;
    float subjectWeight(void) const;
    QString subjectPhenotype(void) const;
    QString studyModel(void) const;
    
    QString anesthesia(void) const;
    QString analgesia(void) const;
    QString otherDrugs(void) const;
    QString foodAccess(void) const;
    QString waterAccess(void) const;

    QDateTime scanTimeQt(void) const;
    QDateTime injectionTimeQt(void) const;
    QString strSubjectWeightUnits(void) const;
    QString strSubjectLengthUnits(void) const;
    QString strDoseUnits(void) const;

    //mutator methods
    void setModel(const int value);
    void setInstitution(const QString value);
    void setFileName(const QString value);
    void setFileType(const int value);
    void setAcquisitionMode(const int value);
    void setBedMotion(const int value);
    void setNumFrames(const int value);
    
    void setIsotope(const QString value);
    void setIsotopeHalftime(const float value);
    void setIsotopeBranchingFraction(const float value);
    
    void setTransaxialCrystalsPerBlock(const int value);
    void setAxialCrystalsPerBlock(const int value);
    void setIntrinsicCrystalOffset(const int value);
    void setAxialBlocks(const int value);
    void setAxialCrystalPitch(const float value);
    void setRadius(const float value);
    void setRadialFov(const float value);
    void setSrcRadius(const float value);
    void setSrcCmPerRev(const float value);
    void setTxSrcType(const int value);
    void setTransaxialBinSize(const float value);
    void setAxialPlaneSize(const float value);
    void setLld(const float value);
    void setUld(const float value);
    
    void setDataType(const int value);
    void setDataOrder(const int value);
    void setSpan(const int value);
    void setRingDifference(const int value);
    void setNumberOfDimensions(const int value);
    void setXDimension(const int value);
    void setYDimension(const int value);
    void setZDimension(const int value);
    void setWDimension(const int value);
    void setDeltaElements(const int i, const int value);
    
    void setDeadtimeCorrectionApplied(const int value);
    void setDecayCorrectionApplied(const int value);
    void setNormalizationApplied(const int value);
    void setAttenuationApplied(const int value);
    void setScatterCorrection(const int value);
    void setArcCorrectionApplied(const int value);

    void setPixelSize(const float value);
    
    void setCalibrationUnits(const int value);
    void setCalibrationFactor(const float value);
    void setCalibrationBranchingFraction(const float value);
    void setNumberOfSinglesRates(const int value);
    
    void setInvestigatorName(const QString value);
    void setOperatorName(const QString value);
    void setStudyIdentifier(const QString value);
    void setScanTime(const long value);
    void setInjectedCompound(const QString value);
    void setDoseUnits(const int value);
    void setDose(const float value);
    void setInjectionTime(const long value);
    void setInjectionDecayCorrection(const float value);
    
    void setSubjectIdentifier(const QString value);
    void setSubjectGenus(const QString value);
    void setSubjectOrientation(const int value);
    void setSubjectLength_units(const int value);
    void setSubjectLength(const float value);
    void setSubjectWeight_units(const int value);
    void setSubjectWeight(const float value);
    void setSubjectPhenotype(const QString value);
    void setStudyModel(const QString value);
    
    void setAnesthesia(const QString value);
    void setAnalgesia(const QString value);
    void setOtherDrugs(const QString value);
    void setFoodAccess(const QString value);
    void setWaterAccess(const QString value);

  protected:
    // convert methods
    bool convertFrom(const CMedIOHeader* mainHeader, const CMedIOHeader* subHeader=NULL);

  private:
    CConcordeMainHeaderPrivate* m_pData;
};

#endif
