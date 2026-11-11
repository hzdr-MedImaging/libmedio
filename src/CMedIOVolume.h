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

//! @file CMedIOVolume.h
//! @brief contains definition of the class CMedIOVolume

#ifndef CMEDIOVOLUME_H
#define CMEDIOVOLUME_H

#include <QByteArray>
#include <QString>

// forward declarations
class CMedIOVolumePrivate;

//! @class CMedIOVolume
//! @brief format independent 3D image volume (one frame)
//!
//! A CMedIOVolume holds the voxel data of one frame of an image file
//! together with the information needed to interpret it, independent of the
//! file format it was read from:
//!
//! - nx x ny x nz voxels in the byte order of this machine, x running fastest
//! - the data type of the stored values and their scaling to physical values:
//!   physical value = stored value x scaleFactor(plane) + intercept()
//! - the geometry as affine transformation of voxel (i, j, k), 0-based, into
//!   DICOM patient coordinates (LPS, mm): x towards the patient's left, y
//!   towards posterior, z towards superior
//! - the frame timing (s)
//!
//! It is the common representation used by CMedIOData::readMatrix() and
//! CMedIOData::writeMatrix() to convert matrix data between the file formats
//! and is returned by CMedIOData::readVolume().
class CMedIOVolume {
  public:
    //! @enum data type of the stored values
    enum DataType { UnknownDataType = 0,
                    UInt8,      //!< unsigned 8 bit integer
                    Int8,       //!< signed 8 bit integer
                    Int16,      //!< signed 16 bit integer
                    UInt16,     //!< unsigned 16 bit integer
                    Int32,      //!< signed 32 bit integer
                    UInt32,     //!< unsigned 32 bit integer
                    Float32,    //!< 32 bit IEEE float
                    Float64     //!< 64 bit IEEE float
                  };

    CMedIOVolume();
    CMedIOVolume(const CMedIOVolume& src);
    CMedIOVolume& operator=(const CMedIOVolume& src);
    ~CMedIOVolume();

    //! remove the data and reset all values
    void clear();
    bool isEmpty() const;

    //! size of a stored value of the data type in bytes (0: unknown)
    static int bytesPerVoxel(CMedIOVolume::DataType type);

    //! dimensions (axis 0..2)
    int dimension(int axis) const;
    qint64 voxelCount() const;

    //! voxel data: data.size() has to be nx * ny * nz * bytesPerVoxel(type);
    //! the scale factors are reset to 1, the intercept to 0
    bool setData(int nx, int ny, int nz, CMedIOVolume::DataType type, const QByteArray& data);
    CMedIOVolume::DataType dataType() const;
    const QByteArray& data() const;

    //! stored and physical value of voxel index (i + nx * (j + ny * k))
    double storedValue(qint64 index) const;
    double value(qint64 index) const;

    //! scaling: physical value = stored value x scaleFactor(plane) + intercept()
    void setScaleFactor(double factor);              // all planes
    void setScaleFactor(int plane, double factor);   // plane k (0-based)
    double scaleFactor(int plane = 0) const;
    bool hasUniformScaleFactor() const;
    void setIntercept(double intercept);
    double intercept() const;

    //! convert the stored values into physical values (Float32, or Float64 if
    //! the data is Float64), scale factor 1 and intercept 0; false (volume
    //! unchanged) if the physical values do not fit into the voxel data of a
    //! volume (a QByteArray, i.e. less than 2 GB)
    bool convertToPhysicalValues();

    //! geometry: affine m (3 x 4) transforming voxel (i, j, k) into LPS (mm)
    void setAffine(const double m[3][4]);
    void affine(double m[3][4]) const;
    double affine(int row, int column) const;
    void position(double i, double j, double k, double p[3]) const;
    double voxelSize(int axis) const;

    //! orientation code of the voxel axes: for every axis the patient
    //! direction it points to (closest axis for oblique data), e.g. "LPI"
    //! for ECAT7 images (x towards left, y towards posterior, planes from
    //! superior to inferior) or "RAS" (nibabel convention)
    QString orientation() const;

    //! reorder the voxels so that the voxel axes point to the patient
    //! directions of the given orientation code (e.g. "LPI"); the geometry
    //! is adjusted, so every voxel keeps its position. Oblique axes are
    //! assigned to the closest patient axis (oblique is set to true). Planes
    //! with different scale factors that do not stay planes are converted
    //! into physical values first (false if that is not possible, see
    //! convertToPhysicalValues()).
    bool reorient(const QString& orientation, bool* oblique = NULL);

    //! frame timing (s)
    void setFrameStart(double seconds);
    double frameStart() const;
    void setFrameDuration(double seconds);
    double frameDuration() const;

  private:
    CMedIOVolumePrivate* m_pData;
};

#endif // CMEDIOVOLUME_H
