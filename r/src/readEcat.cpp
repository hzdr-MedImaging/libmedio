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

#include <vector>
#include <limits>

#include <QFileInfo>

#include <CECAT7MainHeader>
#include <CECAT7SubHeaderImage>

#include "readEcat.h"
#include "CRECATFile.h"

typedef enum
{
  FRAMES = 'f',
  GATES = 'g',
  BEDS = 'b'
} as_volume;

RcppExport SEXP readEcat(SEXP vfile,
                         SEXP indicesOfVolumes,
                         SEXP indicesOfRows,
                         SEXP indicesOfCols,
                         SEXP indicesOfPlanes)
{
  BEGIN_RCPP

  Rcpp::List rList;
  std::vector<short> volumes;
  std::vector<short> rows;
  std::vector<short> cols;
  std::vector<short> planes;

  if(Rcpp::RObject(indicesOfVolumes).isNULL() == false)
    volumes = Rcpp::as<std::vector<short> >(indicesOfVolumes);

  if(Rcpp::RObject(indicesOfRows).isNULL() == false)
    rows = Rcpp::as<std::vector<short> >(indicesOfRows);

  if(Rcpp::RObject(indicesOfCols).isNULL() == false)
    cols = Rcpp::as<std::vector<short> >(indicesOfCols);

  if(Rcpp::RObject(indicesOfPlanes).isNULL() == false)
    planes = Rcpp::as<std::vector<short> >(indicesOfPlanes);

  // if vfile was NOT supplied as well (empty readEcat() call)
  // then we simply return a template structure (empty main header,
  // one empty frame + subheader) user can fill in their data
  if(Rcpp::RObject(vfile).isNULL() == true)
	{
		// create an empty matrixData object
    Rcpp::Dimension dim(10, 10, 10);
    Rcpp::NumericVector matrixData(dim);

    // create a temp object
    CRECATFile ecatFile(CECATMainHeader::ECAT7_Volume16);

    // we create an empty main and sub header here
    Rcpp::List mhead = Rcpp::as<Rcpp::List>(ecatFile.createEmptyMainHeader());
    Rcpp::List shead = Rcpp::as<Rcpp::List>(ecatFile.createEmptySubHeader());

    // lets fill the main header with sensible data
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
    shead["x_dimension"] = dim[1]; // y -> x
    shead["y_dimension"] = dim[0]; // x -> y
    shead["z_dimension"] = dim[2]; // z -> z
    shead["x_pixelsize"] = 1.0; // y -> x
    shead["y_pixelsize"] = 1.0; // x -> y
    shead["z_pixelsize"] = 1.0; // z -> z
    mhead["bin_size"] = dim[2];

    matrixData.attr("class") = Rcpp::StringVector::create("Volume", "array");
    matrixData.attr("shead") = shead;
    matrixData.attr("voxsiz") = Rcpp::NumericVector::create(shead["y_pixelsize"],
                                                            shead["x_pixelsize"],
                                                            shead["z_pixelsize"]);

    matrixData.attr("vgrid") = Rcpp::List::create(Rcpp::Named("rows") = 10,
                                                  Rcpp::Named("cols") = 10,
                                                  Rcpp::Named("planes") = 10);

    rList.push_back(matrixData, "f1");

    // create the attributes of the ecat structur
    rList.attr("class") = "Ecat";
    rList.attr("mhead") = mhead;
  }
  else
  {
    QString filename = Rcpp::as<std::string>(vfile).c_str();
    CRECATFile inputFile(filename);

    if(inputFile.open(QIODevice::ReadOnly) &&
       (inputFile.format() != CECATFile::Undefined) )
    {
      Rcpp::List mhead;

      if(inputFile.readMainHeader(mhead) == true)
      {
        int num_frames = Rcpp::as<int>(mhead["num_frames"]);
        int num_gates = Rcpp::as<int>(mhead["num_gates"]);
        int num_bed_pos = Rcpp::as<int>(mhead["num_bed_pos"]);
        float ecat_calibration_factor = Rcpp::as<float>(mhead["ecat_calibration_factor"]);

        as_volume which_to_use;

        short frame = 1;
        short gate = 1;
        short bed = 0;

        int num_volumes;
        // now we have to check wether to use frames, gates or beds as volumes
        if( (num_frames > 1) && (num_gates <= 1) && ((num_bed_pos+1) <= 1) )
        {
          // use frames
          which_to_use = FRAMES;
          num_volumes = num_frames;
        }
        else if( (num_frames <= 1) && (num_gates > 1) && ((num_bed_pos+1) <= 1) )
        {
          // use gates
          which_to_use = GATES;
          num_volumes = num_gates;
        }
        else if( (num_frames <= 1) && (num_gates <= 1) && ((num_bed_pos+1) > 1) )
        {
          // use beds
          which_to_use = BEDS;
          num_volumes = num_bed_pos + 1;
        }
        else if( (num_frames <= 1) && (num_gates <=1) && ((num_bed_pos+1) <= 1) )
        {
          // if there is none of them bigger then 1,
          // we use frames
          which_to_use = FRAMES;
          num_volumes = num_frames;
        }
        else
        {
          // ... at least we would have no idea how to assign the
          // indices to the volumes...
          Rcpp::Rcerr << "ERROR: can handle only a single dynamic index (one of  frame/gate/bed!" << std::endl;
          return rList;
        }

        // check if volumes index array is given
        // we take all frames if nothing is specified
        if(volumes.size() == 0)
        {
          for(int i = 1; i <= num_volumes; ++i)
            volumes.push_back(i);
        }
        else
        {
          // we have to check if the index std::vector exceeds the boundaries
          std::vector<short>::iterator it;
          for(it = volumes.begin(); it < volumes.end(); ++it)
          {
            if( (*it < 1) || (*it > num_volumes))
            {
              Rcpp::Rcerr << "ERROR: index in volumes std::vector is exceeding boundaries "
                   << "there are " << num_volumes << " volumes in this file." << std::endl;
              return rList;
            }
          }
        }

        Rcpp::NumericVector frame_midpoint;
        Rcpp::StringVector volume_tags;

        std::vector<short>::iterator it;
        for(it = volumes.begin(); it != volumes.end(); ++it)
        {
          char* byteData = NULL;
          unsigned int byteDataSize = 0;
          Rcpp::List subHeader;

          switch(which_to_use)
          {
            case FRAMES: frame = *it; break;
            case GATES: gate = *it; break;
            case BEDS: bed = *it - 1; break; // numbering of beds starts at 0
          }

          if(inputFile.readSubHeader(subHeader, frame, 1, gate, bed, 0) == true)
          {
            int data_type = Rcpp::as<int>(subHeader["data_type"]);
            short x_dimension = Rcpp::as<short>(subHeader["x_dimension"]);
            short y_dimension = Rcpp::as<short>(subHeader["y_dimension"]);
            short z_dimension = Rcpp::as<short>(subHeader["z_dimension"]);
            float scale_factor = Rcpp::as<float>(subHeader["scale_factor"]);
            float x_pixelsize = Rcpp::as<float>(subHeader["x_pixelsize"]);
            float y_pixelsize = Rcpp::as<float>(subHeader["y_pixelsize"]);
            float z_pixelsize = Rcpp::as<float>(subHeader["z_pixelsize"]);

            // check if rows, cols and/or planes index arrays are given
            // if not take all of them
            bool exceeding_boundaries = false;
            if(rows.size() == 0)
            {
              for(int i = 1; i <= y_dimension; i++)
                rows.push_back(i);
            }
            else
            {
              // we have to check if the index std::vector exceeds the boundaries
              std::vector<short>::iterator it;
              for(it = rows.begin(); it < rows.end(); ++it)
              {
                if( (*it < 1) || (*it > y_dimension))
                {
                  Rcpp::Rcerr << "ERROR: index in rows vector is out of bounds "
                       << "there are " << y_dimension << " rows in this file." << std::endl;
                  exceeding_boundaries = true;
                }
              }
            }

            if(cols.size() == 0)
            {
              for(int i = 1; i <= x_dimension; i++)
                cols.push_back(i);
            }
            else
            {
              // we have to check if the index std::vector exceeds the boundaries
              std::vector<short>::iterator it;
              for(it = cols.begin(); it < cols.end(); ++it)
              {
                if( (*it < 1) || (*it > x_dimension))
                {
                  Rcpp::Rcerr << "ERROR: index in cols vector is out of bounds "
                       << "there are " << x_dimension << " cols in this file." << std::endl;
                  exceeding_boundaries = true;
                }
              }
            }

            if(planes.size() == 0)
            {
              for(int i = 1; i <= z_dimension; i++)
                planes.push_back(i);
            }
            else
            {
              // we have to check if the index std::vector exceeds the boundaries
              std::vector<short>::iterator it;
              for(it = planes.begin(); it < planes.end(); ++it)
              {
                if( (*it < 1) || (*it > z_dimension))
                {
                  Rcpp::Rcerr << "ERROR: index in planes std::vector is out of bounds "
                       << "there are " << z_dimension << " planes in this file." << std::endl;
                  exceeding_boundaries = true;
                }
              }
            }

            if(exceeding_boundaries == true)
              break;

            if(inputFile.readMatrix(byteData, byteDataSize, frame, 1, gate, bed, 0) == true)
            {
              Rcpp::Dimension dim(rows.size(), cols.size(), planes.size());
              Rcpp::NumericVector matrixData(dim);

              matrixData.attr("class") = Rcpp::StringVector::create("Volume", "array");
              matrixData.attr("shead") = subHeader;
              matrixData.attr("voxsiz") = Rcpp::NumericVector::create(y_pixelsize,
                                                                      x_pixelsize,
                                                                      z_pixelsize);

              matrixData.attr("vgrid") = Rcpp::List::create(Rcpp::Named("rows") = rows,
                                                            Rcpp::Named("cols") = cols,
                                                            Rcpp::Named("planes") = planes);

              // lets walk trhough our Plane,Row,Column iterators
              // and extract the correct data from our byteData array
              int plane_size = x_dimension * y_dimension;

              // in cases were we only want some slices of the data
              // our destination matrix is smaller (equivalent to the length
              // of the index std::vectors)
              // and we have to flip x and y
              int dst_x_dimension = cols.size();
              int dst_y_dimension = rows.size();
              int dst_plane_size  = dst_x_dimension * dst_y_dimension;

              // extract the data size of data_type
              int data_size = 0; // 0 == error
              switch(data_type)
              {
                case CECATSubHeader::ByteData:
                  data_size = sizeof(qint8);
                break;

                case CECATSubHeader::SunShort:
                  data_size = sizeof(qint16);
                break;

                case CECATSubHeader::IEEEFloat:
                  data_size = sizeof(float);
                break;

                case CECATSubHeader::SunLong:
                  data_size = sizeof(qint32);
                break;
              }

              // check if a support data type was found
              if(data_size > 0)
              {
                void* mData = byteData;

                // iterate through our index std::vectors
                // and copy only the relevant data
                std::vector<short>::iterator itPlane = planes.begin();
                for(int z = 0; itPlane < planes.end(); ++itPlane, ++z)
                {
                  std::vector<short>::iterator itRow = rows.begin();
                  for(int y = 0; itRow < rows.end(); ++itRow, ++y)
                  {
                    std::vector<short>::iterator itColumn = cols.begin();
                    for(int x = 0; itColumn < cols.end(); ++itColumn, ++x)
                    {
                      // calculate the srcIndex in our loaded byteData and the dstIndex
                      // to put the data into our matrixData ending up in R
                      unsigned int srcIndex = (*itRow-1) * x_dimension + (*itColumn-1) + (*itPlane-1) * plane_size;
                      unsigned int dstIndex = x * dst_y_dimension + y + z * dst_plane_size;

                      #if defined(DEBUG)
                      Rcpp::Rcout << "x/y/z: " << (*itColumn-1) << "/" << (*itRow-1) << "/" << (*itPlane-1) << " -> " << x << "/" << y << "/" << z << " srcIndex: " << srcIndex << " dstIndex: " << dstIndex << " size: " << matrixData.size() << " value: ";
                      #endif

                      // now depending on the data type of the src data we have to convert
                      // differently to float values
                      if(data_type == CECATSubHeader::ByteData)
                      {
                        qint8* pint8 = static_cast<qint8*>(mData);
                        matrixData[dstIndex] = static_cast<float>(pint8[srcIndex]) * scale_factor * ecat_calibration_factor;
                        #if defined(DEBUG)
                        Rcpp::Rcout << matrixData[dstIndex] << std::endl;
                        #endif
                      }
                      else if(data_type == CECATSubHeader::SunShort)
                      {
                        qint16* pint16 = static_cast<qint16*>(mData);
                        matrixData[dstIndex] = static_cast<float>(pint16[srcIndex]) * scale_factor * ecat_calibration_factor;
                        #if defined(DEBUG)
                        Rcpp::Rcout << matrixData[dstIndex] << std::endl;
                        #endif
                      }
                      else if(data_type == CECATSubHeader::IEEEFloat)
                      {
                        float* pfloat = static_cast<float*>(mData);
                        matrixData[dstIndex] = static_cast<float>(pfloat[srcIndex]) * scale_factor * ecat_calibration_factor;
                        #if defined(DEBUG)
                        Rcpp::Rcout << matrixData[dstIndex] << std::endl;
                        #endif
                      }
                      else if(data_type == CECATSubHeader::SunLong)
                      {
                        qint32* pint32 = static_cast<qint32*>(mData);
                        matrixData[dstIndex] = static_cast<float>(pint32[srcIndex]) * scale_factor * ecat_calibration_factor;
                        #if defined(DEBUG)
                        Rcpp::Rcout << matrixData[dstIndex] << std::endl;
                        #endif
                      }
                    }
                  }
                }
              }
              else
                Rcpp::Rcerr << "ERROR: data_type " << data_type << " currently not supported by Rmedio" << std::endl;

              QString tag(which_to_use);
              tag.append(QString::number(*it));
              volume_tags.push_back(tag.toLatin1().constData());
              rList.push_back(matrixData, tag.toLatin1().constData());

              // calculate the frame midpoint of this volume
              unsigned int frame_duration = Rcpp::as<unsigned int>(subHeader["frame_duration"]) / 1000; // [s]
              unsigned int frame_start = Rcpp::as<unsigned int>(subHeader["frame_start"]) / 1000; // [s]
              double fm = frame_start + frame_duration / 2;
              frame_midpoint.push_back(fm);

              delete[] byteData;
            }
            else
              Rcpp::Rcerr << "ERROR: can't read matrix data" << std::endl;
          }
          else
            Rcpp::Rcerr << "ERROR: can't read the sub header" << std::endl;
        }

        // set the volume names of the midpoints
        frame_midpoint.attr("names") = volume_tags;

        // create the attributes of the ecat structur
        rList.attr("class") = "Ecat";
        rList.attr("mhead") = mhead;
        rList.attr("frame_midpoint") = frame_midpoint;
      }
      else
        Rcpp::Rcerr << "ERROR: can't read main header" << std::endl;

      inputFile.close();
    }
    else
      Rcpp::Rcerr << "cannot open file '" << filename.toLatin1().constData()
           << "': there is no such ecat file" << std::endl;
  }

  return rList;

  END_RCPP
}
