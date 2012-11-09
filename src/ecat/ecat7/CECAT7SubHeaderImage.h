/* -*- mode: c++; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 * vim:set ts=2 sw=2 expandtab: *********************************************
 *
 * libmedio - C++ I/O Library for loading/saving medical data formats
 * Copyright (C) 2006-2010 by Jens Langner <Jens.Langner@light-speed.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id$
 *
 **************************************************************************/

#ifndef CECAT7SUBHEADERIMAGE_H
#define CECAT7SUBHEADERIMAGE_H

#include <QDataStream>

#ifndef __MEDIO_PRIVATE__
#include <CECATSubHeader>
#else
#include <CECATSubHeader.h>
#endif

// forward declarations
class CECAT7SubHeaderImagePrivate;
class CECATFile;
class CECATDirectoryItem;

class CECAT7SubHeaderImage : public CECATSubHeader
{
  public:
    // public enumeration types
    enum Filter_Code      {  NoFilter=0,
                            Ramp,
                            Butterfield,
                            Hanning,
                            Hamming,
                            Parzen,
                            Shepp,
                            Butterworth,
                            Gaussian,
                            Median,
                            Boxcar };

    enum Scatter_Type      {  None=0,
                            Deconvolution,
                            Simulated,
                            DualEnergy };

    enum Recon_Type        {  FilteredBackProjection=0,
                            ForwardProjection3D,
                            Ramp3D,
                            Favor3D,
                            SSRB,
                            MultiSliceRebinning,
                            FORE,
                            BasicOsem,
                            CPUReconBase,
                            FourierCPURecon,
                            FilteredBackProjectionCPURecon,
                            IterativeCPURecon,
                            FourierCPUReconSmallFOV,
                            FilteredBackProjectionCPUReconSmallFOV,
                                                                IterativeCPUReconSmallFOV };

    // public BitMask field
    enum Processing_Code  {  NotProcessed        = (0<<0),
                            Normalized          = (1<<0),
                            MeasuredAttenCorr    = (1<<1),
                            CalculatedAttenCorr = (1<<2),
                            XSmoothing          = (1<<3),
                            YSmoothing          = (1<<4),
                            ZSmoothing          = (1<<5),
                            Scatter2DCorr        = (1<<6),
                            Scatter3DCorr        =  (1<<7),
                            ArcCorrection        =  (1<<8),
                            DecayCorrection      =  (1<<9),
                                                                            OnlineCompression    = (1<<10),
                            FourierRebinning      =  (1<<11),
                            SingleSliceRebinning      =  (1<<12),
                            SegmentZeroOnly    = (1<<13) };

    // constructors
    CECAT7SubHeaderImage(CECATFile* ecatFile = NULL,
                         CECATDirectoryItem* pDirItem = NULL);
    ~CECAT7SubHeaderImage();

    // copy constructur and default assignment operator
    CECAT7SubHeaderImage(const CECAT7SubHeaderImage& src);    
    CECAT7SubHeaderImage& operator=(const CECAT7SubHeaderImage& src);

    // header clear method
    void clear();

    // public methods
    bool load(void);
    bool save(void) const;

    // the number of bytes the data of that header requires on disk
    int rawDataSize() const;
    
    CECATSubHeader::Type subHeaderType(void) const;

    // clone methods
    CMedIOHeader* clone() const;

    // conversion methods
    bool convertFrom(const CMedIOHeader* pHead);

    // data acess methods
    CECATSubHeader::Data_Type data_Type(void) const;
    short num_Dimensions(void) const;
    short x_Dimension(void) const;
    short y_Dimension(void) const;
    short z_Dimension(void) const;
    float x_Offset(void) const;
    float y_Offset(void) const;
    float z_Offset(void) const;
    float recon_Zoom(void) const;
    float scale_Factor(void) const;
    short image_Min(void) const;
    short image_Max(void) const;
    float x_Pixel_Size(void) const;
    float y_Pixel_Size(void) const;
    float z_Pixel_Size(void) const;
    unsigned int frame_Duration(void) const;
    unsigned int frame_Start_Time(void) const;
    Filter_Code filter_Code(void) const;
    float x_Resolution(void) const;
    float y_Resolution(void) const;
    float z_Resolution(void) const;
    float num_R_Elements(void) const;
    float num_Angles(void) const;
    float z_Rotation_Angle(void) const;
    float decay_Corr_Fctr(void) const;
    unsigned int processing_Code(void) const;
    unsigned int gate_Duration(void) const;
    unsigned int r_Wave_Offset(void) const;
    unsigned int num_Accepted_Beats(void) const;
    float filter_Cutoff_Frequency(void) const;
    float filter_Resolution(void) const;
    float filter_Ramp_Slope(void) const;
    short filter_Order(void) const;
    float filter_Scatter_Fraction(void) const;
    float filter_Scatter_Slope(void) const;
    const char* annotation(void) const;
    float mt_1_1(void) const;
    float mt_1_2(void) const;
    float mt_1_3(void) const;
    float mt_2_1(void) const;
    float mt_2_2(void) const;
    float mt_2_3(void) const;
    float mt_3_1(void) const;
    float mt_3_2(void) const;
    float mt_3_3(void) const;
    float rfilter_Cutoff(void) const;
    float rfilter_Resolution(void) const;
    Filter_Code rfilter_Code(void) const;
    short rfilter_Order(void) const;
    float zfilter_Cutoff(void) const;
    float zfilter_Resolution(void) const;
    Filter_Code zfilter_Code(void) const;
    short zfilter_Order(void) const;
    float mt_1_4(void) const;
    float mt_2_4(void) const;
    float mt_3_4(void) const;
    Scatter_Type scatter_Type(void) const;
    Recon_Type recon_Type(void) const;
    short recon_Views(void) const;
    short cti_Reserved(const short i) const;
    short user_Reserved(const short i) const;

    // methods to modify elements of the SubHeader
    void setData_Type(const CECATSubHeader::Data_Type dType);
    void setNum_Dimensions(const short dim);
    void setX_Dimension(const short dim);
    void setY_Dimension(const short dim);
    void setZ_Dimension(const short dim);
    void setX_Offset(const float off);
    void setY_Offset(const float off);
    void setZ_Offset(const float off);
    void setRecon_Zoom(const float zoom);
    void setScale_Factor(const float scale);
    void setImage_Min(const short min);
    void setImage_Max(const short max);
    void setX_Pixel_Size(const float size);
    void setY_Pixel_Size(const float size);
    void setZ_Pixel_Size(const float size);
    void setFrame_Duration(const unsigned int dur);
    void setFrame_Start_Time(const unsigned int time);
    void setFilter_Code(const Filter_Code code);
    void setX_Resolution(const float res);
    void setY_Resolution(const float res);
    void setZ_Resolution(const float res);
    void setNum_R_Elements(const float elem);
    void setNum_Angles(const float angles);
    void setZ_Rotation_Angle(const float angle);
    void setDecay_Corr_Fctr(const float factor);
    void setProcessing_Code(const unsigned int code);
    void setGate_Duration(const unsigned int dur);
    void setR_Wave_Offset(const unsigned int off);
    void setNum_Accepted_Beats(const unsigned int beats);
    void setFilter_Cutoff_Frequency(const float freq);
    void setFilter_Resolution(const float res);
    void setFilter_Ramp_Slope(const float slope);
    void setFilter_Order(const short order);
    void setFilter_Scatter_Fraction(const float frac);
    void setFilter_Scatter_Slope(const float slope);
    void setAnnotation(const char* str);
    void setMT_1_1(const float value);
    void setMT_1_2(const float value);
    void setMT_1_3(const float value);
    void setMT_2_1(const float value);
    void setMT_2_2(const float value);
    void setMT_2_3(const float value);
    void setMT_3_1(const float value);
    void setMT_3_2(const float value);
    void setMT_3_3(const float value);
    void setRFilter_Cutoff(const float cutoff);
    void setRFilter_Resolution(const float res);
    void setRFilter_Code(const Filter_Code code);
    void setRFilter_Order(const short order);
    void setZFilter_Cutoff(const float cutoff);
    void setZFilter_Resolution(const float res);
    void setZFilter_Code(const Filter_Code code);
    void setZFilter_Order(const short order);
    void setMT_1_4(const float value);
    void setMT_2_4(const float value);
    void setMT_3_4(const float value);
    void setScatter_Type(const Scatter_Type type);
    void setRecon_Type(const Recon_Type type);
    void setRecon_Views(const short views);
    void setCTI_Reserved(const short i, const short n);
    void setUser_Reserved(const short i, const short n);

    // special conversion routines
    float suv_Scale_Factor(bool& ok) const;
    
  private:
    CECAT7SubHeaderImagePrivate* m_pData;
};

#endif // CECAT7SUBHEADERIMAGE_H
