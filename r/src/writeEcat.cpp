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

#include <Rcpp.h>

#include <cmath>
#include <limits>

#include <QFileInfo>

#include <CECAT7MainHeader>
#include <CECAT7SubHeaderImage>

#include "writeEcat.h"
#include "CRECATFile.h"

RcppExport SEXP writeEcat(SEXP ecat, SEXP vfilename, SEXP overwrite, SEXP dtype)
{
  BEGIN_RCPP

  bool result = true;
  Rcpp::List RcppEcatFile;
  QString outputFileName;
  bool overwriteFile;
  CECATSubHeader::Data_Type forcedDataType = CECATSubHeader::UnknownDataType;

  Rcpp::List mhead;
  Rcpp::List shead;
  Rcpp::NumericVector frame_midpoint;

  if(Rcpp::RObject(vfilename).isNULL())
  {
    Rcpp::Rcerr << "ERROR: no output file name given." << std::endl;
    result = false;
  }
  else
    outputFileName = Rcpp::as<std::string>(vfilename).c_str();

  if(Rcpp::RObject(ecat).isNULL())
  {
    Rcpp::Rcerr << "ERROR: no volume data given." << std::endl;
    result = false;
  }
  else
  {
    // now we check whether 'ecat' is an object of class 'Ecat' or not
    if(Rcpp::RObject(ecat).inherits("Ecat"))
    {
      // create a copy (clone) of the R object or otherwise we are
      // modifying the original object during our processing
      RcppEcatFile = clone(Rcpp::as<Rcpp::List>(ecat));
    }
    else
    {
      // convert the ecat object to a numeric vector. We have to use clone() here
      // or otherwise we are going to convert the original object in R as well.
      Rcpp::NumericVector matrixData = clone(Rcpp::as<Rcpp::NumericVector>(ecat));

      // create a temp object
      CRECATFile ecatFile(CECATMainHeader::ECAT7_Volume16);

      // we create an empty main and sub header here
      Rcpp::List mhead = Rcpp::as<Rcpp::List>(ecatFile.createEmptyMainHeader());
      Rcpp::List shead = Rcpp::as<Rcpp::List>(ecatFile.createEmptySubHeader());

      // lets fill the main header with sensible data
      mhead["original_file_name"] = vfilename;
      mhead["isotope_name"] = "F-18";
      mhead["isotope_halflife"] = 6586.2;
      mhead["radiopharmaceutical"] = "F-18 FDG";
      mhead["num_frames"] = 1;
      mhead["ecat_calibration_factor"] = 1.0;
      mhead["patient_orientation"] = static_cast<short>(CECAT7MainHeader::HF_Prone);

      // lets fill the sub header with sensible data
      shead["data_type"] = static_cast<short>(CECATSubHeader::IEEEFloat); // use float per default
      shead["num_dimensions"] = 3;
      shead["recon_zoom"] = 1;
      Rcpp::Dimension dim = Rcpp::as<Rcpp::Dimension>(matrixData.attr("dim"));
      shead["x_dimension"] = dim[1]; // y -> x
      shead["y_dimension"] = dim[0]; // x -> y
      shead["z_dimension"] = dim[2]; // z -> z
      mhead["bin_size"] = dim[2];

      // check if user has specified a voxsiz attribute
      if(Rf_isNull(matrixData.attr("voxsiz")))
      {
        // assume voxelsize 1,1,1 if no voxsiz attribute has been given.
        shead["x_pixelsize"] = 1;
        shead["y_pixelsize"] = 1;
        shead["z_pixelsize"] = 1;
      }
      else
      {
        Rcpp::NumericVector voxsiz = Rcpp::as<Rcpp::NumericVector>(matrixData.attr("voxsiz"));
        shead["x_pixelsize"] = voxsiz[1]; // y -> x
        shead["y_pixelsize"] = voxsiz[0]; // x -> y
        shead["z_pixelsize"] = voxsiz[2]; // z -> z
      }

      shead["num_r_elements"] = 288;
      shead["num_angles"] = 144;

      // convert the volume data to a numericvector
      matrixData.attr("class") = Rcpp::StringVector::create("Volume", "array");
      matrixData.attr("shead") = shead;

      // add the converted matrixData to our Ecat class
      RcppEcatFile.push_back(matrixData, "f1");

      // if not we construct a dummy Ecat object instead and use that instead
      RcppEcatFile.attr("class") = "Ecat";
      RcppEcatFile.attr("mhead") = mhead;
      frame_midpoint.push_back(0.0);
      frame_midpoint.attr("names") = Rcpp::StringVector::create("f1");
      RcppEcatFile.attr("frame_midpoint") = frame_midpoint;
    }
  }

  if(Rcpp::RObject(overwrite).isNULL())
  {
    // we do not overwrite an existing file
    // on default
    overwriteFile = false;
  }
  else
    overwriteFile = Rcpp::as<bool>(overwrite);

  if(result == true)
  {
    if(QFileInfo(outputFileName).exists())
    {
      if(overwriteFile == false)
      {
        Rcpp::Rcerr << "ERROR: output file '" << outputFileName.toLatin1().constData() << "' already exists. Use 'owrite=TRUE'." << std::endl;
        result = false;
      }
      else
        QFile(outputFileName).remove();
    }
  }

  // check if the user specified another data type
  if(Rcpp::RObject(dtype).isNULL())
    forcedDataType = CECATSubHeader::UnknownDataType;
  else
  {
    QString sdtype = Rcpp::as<std::string>(dtype).c_str();

    if(sdtype.startsWith('s', Qt::CaseInsensitive))
      forcedDataType = CECATSubHeader::SunShort;
    else if(sdtype.startsWith('i', Qt::CaseInsensitive))
      forcedDataType = CECATSubHeader::SunLong;
    else if(sdtype.startsWith('f', Qt::CaseInsensitive))
      forcedDataType = CECATSubHeader::IEEEFloat;
    else
    {
      bool convertOk = false;

      // convert to a number
      forcedDataType = static_cast<CECATSubHeader::Data_Type>(sdtype.toInt(&convertOk));
      if(convertOk == false || forcedDataType < CECATSubHeader::UnknownDataType || forcedDataType > CECATSubHeader::SunLong)
      {
        Rcpp::Rcerr << "ERROR: specified data type '" << sdtype.toLatin1().constData() << "' invalid or not supported." << std::endl;
        result = false;
      }
    }
  }

  if(result == true)
  {
    CRECATFile outputFile(outputFileName, CECATMainHeader::ECAT7_Volume16);

    if(outputFile.open(QIODevice::ReadWrite) && outputFile.format() != CECATFile::Undefined)
    {
      Rcpp::List RcppMainHeader = RcppEcatFile.attr("mhead");
      //int file_type = Rcpp::as<int>(RcppMainHeader("file_type"));

      if(outputFile.writeMainHeader(RcppMainHeader) == false)
      {
        Rcpp::Rcerr << "ERROR: could not write destination main header." << std::endl;
        outputFile.remove();
        result = false;
      }

      // now we write the matrix data to the file
      // therefor we need to read out the names of the volumes
      // and check the first char of the name to know if frames, gates or beds
      // are used as volumes in the R ecat data structure ('f', 'g' or 'b')
      std::vector<std::string> names = Rcpp::as<std::vector<std::string> >(RcppEcatFile.names());
      int numberOfVolumes = names.size();

      short frame = 1;
      short gate = 1;
      short bed = 0;

      if(numberOfVolumes > 0)
      {
        char firstChar = names[0][0];

        for(int volume = 1; volume <= numberOfVolumes; ++volume)
        {
          Rcpp::NumericVector volumeVector = RcppEcatFile[volume-1];
          Rcpp::List rSubHeader = volumeVector.attr("shead");

          // now set frame, gate or bed correct
          switch(firstChar)
          {
            case 'f': frame = volume; break;
            case 'g': gate = volume; break;
            case 'b': bed = volume - 1; break; // numbering of beds starts at 0
            default:
              result = false;
              Rcpp::Rcerr << "ERROR: there is something wrong with the volume names." << std::endl;
          }

          short x_dimension = Rcpp::as<short>(rSubHeader["x_dimension"]);
          short y_dimension = Rcpp::as<short>(rSubHeader["y_dimension"]);
          short z_dimension = Rcpp::as<short>(rSubHeader["z_dimension"]);

          float ecat_calibration_factor = Rcpp::as<float>(RcppMainHeader["ecat_calibration_factor"]);

          int plane_size = x_dimension * y_dimension;
          int dataSize = plane_size * z_dimension;

          // extract the data size of data_type
          CECATSubHeader::Data_Type dataType;
          if(forcedDataType != CECATSubHeader::UnknownDataType)
          {
            dataType = forcedDataType;
            rSubHeader["data_type"] = static_cast<short>(dataType);
          }
          else
            dataType = static_cast<CECATSubHeader::Data_Type>(Rcpp::as<int>(rSubHeader["data_type"]));

          int dataTypeSize = 0;
          switch(dataType)
          {
            case CECATSubHeader::ByteData:
              dataTypeSize = sizeof(qint8);
            break;

            case CECATSubHeader::SunShort:
              dataTypeSize = sizeof(qint16);
            break;

            case CECATSubHeader::IEEEFloat:
              dataTypeSize = sizeof(float);
            break;

            case CECATSubHeader::SunLong:
              dataTypeSize = sizeof(qint32);
            break;

            default:
              dataTypeSize = 0; // 0 == error (not supported)
            break;
          }

          if(dataTypeSize != 0)
          {
            // check if the dimensions are correct
            if(dataSize == volumeVector.size())
            {
              unsigned int matrixSize = dataSize * dataTypeSize;
              char* matrixData = new char[matrixSize];
              float newMin = FLT_MAX;
              float newMax = -FLT_MAX;

              // search for the new min and max values
              for(int index = 0; index < dataSize; ++index)
              {
                float voxel = volumeVector[index] / ecat_calibration_factor;
                newMin = qMin(newMin, voxel);
                newMax = qMax(newMax, voxel);
              }
            
              // calculate the scale factor (only for integer data): like
              // CNIFTIConverter, use the full range of the integer type, so
              // that small values (e.g. after the division by a large
              // calibration factor) keep their precision
              double maxValue = qMax(qAbs(static_cast<double>(newMax)), qAbs(static_cast<double>(newMin)));
              double typeMin = 0.0;
              double typeMax = 0.0;
              switch(dataType)
              {
                case CECATSubHeader::ByteData:
                  typeMin = std::numeric_limits<qint8>::min();
                  typeMax = std::numeric_limits<qint8>::max();
                break;

                case CECATSubHeader::SunShort:
                  typeMin = std::numeric_limits<qint16>::min();
                  typeMax = std::numeric_limits<qint16>::max();
                break;

                case CECATSubHeader::IEEEFloat:
                {
                  newMin = 0.0f;
                  newMax = 0.0f;
                }
                break;

                case CECATSubHeader::SunLong:
                  typeMin = std::numeric_limits<qint32>::min();
                  typeMax = std::numeric_limits<qint32>::max();
                break;

                default:
                  // nothing
                break;
              }

              // the scale factor is stored as float in the sub header,
              // so the stored values are calculated with exactly that value
              float newScaleFactor = 1.0f;
              if(typeMax > 0.0 && maxValue > 0.0)
                newScaleFactor = static_cast<float>(maxValue / typeMax);
 
              // now we iterate through our volumeVector. But we do this
              // here in a fashion that is compatible to the way the ECAT format
              // stores the volume data.
              void* mData = matrixData;
              for(int z = 0; z < z_dimension; ++z) // plane
              {
                for(int y = 0; y < y_dimension; ++y) // row
                {
                  for(int x = 0; x < x_dimension; ++x) // column
                  {
                    // calculate the srcIndex in our volumeVector from R and the dstIndex
                    // to put the data in an ordner the ECAT format defines.
                    unsigned int srcIndex = x * y_dimension + y + z * plane_size;
                    unsigned int dstIndex = y * x_dimension + x + z * plane_size;

                    // calculate the voxel value which we store in the ecat file
                    double val = volumeVector[srcIndex] / newScaleFactor / ecat_calibration_factor;

                    // round integer values and keep them in the range of the
                    // type (float rounding of the scale factor)
                    if(dataType != CECATSubHeader::IEEEFloat)
                      val = qBound(typeMin, std::floor(val + 0.5), typeMax);

                    #if defined(DEBUG)
                    if(x < 20 && y == 0 && z == 0)
                      Rcpp::Rcout << "x/y/z: " << x << "/" << y << "/" << z << " srcIndex: " << srcIndex << " dstIndex: " << dstIndex << std::endl;
                    #endif

                    // now depending on the data type of the src data we have to convert
                    // differently
                    if(dataType == CECATSubHeader::ByteData)
                    {
                      qint8* mData8 = static_cast<qint8*>(mData);
                      mData8[dstIndex] = static_cast<qint8>(val);
                    }
                    else if(dataType == CECATSubHeader::SunShort)
                    {
                      qint16* mData16 = static_cast<qint16*>(mData);
                      mData16[dstIndex] = static_cast<qint16>(val);
                    }
                    else if(dataType == CECATSubHeader::IEEEFloat)
                    {
                      float* mDataflt = static_cast<float*>(mData);
                      mDataflt[dstIndex] = static_cast<float>(val);
                    }
                    else if(dataType == CECATSubHeader::SunLong)
                    {
                      qint32* mData32 = static_cast<qint32*>(mData);
                      mData32[dstIndex] = static_cast<qint32>(val);
                    }
                  }
                }
              }            

              rSubHeader["scale_factor"] = newScaleFactor;
              rSubHeader["image_min"] = static_cast<short>(qRound(newMin / newScaleFactor));
              rSubHeader["image_max"] = static_cast<short>(qRound(newMax / newScaleFactor));

              if(outputFile.writeSubHeader(rSubHeader, frame, 1, gate, bed))
              {
                if(outputFile.writeMatrix(matrixData, matrixSize, frame, 1, gate, bed) == false)
                {
                  Rcpp::Rcerr << "ERROR: writing matrix data failed for volume " << volume << std::endl;
                  delete[] matrixData;
                  outputFile.remove();
                  result = false;
                  break;
                }
              }
              else
              {
                Rcpp::Rcerr << "ERROR: writing subheader of volume " << volume << " failed." << std::endl;
                delete[] matrixData;
                outputFile.remove();
                result = false;
                break;
              }

              delete[] matrixData;
            }
            else
            {
              Rcpp::Rcerr << "ERROR: the dimensions in volume " << volume << " are not correct" << std::endl;
              outputFile.remove();
              result = false;
              break;
            }
          }
          else
          {
            Rcpp::Rcerr << "ERROR: data type " << dataType << " is not supported." << std::endl;
            outputFile.remove();
            result = false;
            break;
          }
        }
      }
      else
      {
        Rcpp::Rcerr << "ERROR: there is no volume data in the ecat data." << std::endl;
        outputFile.remove();
        result = false;
      }

      outputFile.close();
    }
    else
    {
      Rcpp::Rcerr << "ERROR: could not open destination file." << std::endl;
      result = false;
    }
  }

  if(result == true)
    return Rcpp::wrap(RcppEcatFile);
  else
    return Rcpp::wrap(0);

  END_RCPP
}
