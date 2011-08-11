#include "RInterface.h"
#include <iostream>
#include <QFileInfo>
#include <Rcpp.h>

#include <float.h>
#include <limits.h>

#include "CECAT7MainHeader.h"
#include "CECAT7SubHeaderImage.h"
#include "CRECATFile.h"

using namespace std;

RcppExport SEXP readEcat(SEXP vfile,
                         SEXP indicesOfVolumes,
                         SEXP indicesOfRows,
                         SEXP indicesOfCols,
                         SEXP indicesOfPlanes)
{
  BEGIN_RCPP

  Rcpp::List rList;
  QString filename;
  vector<short> volumes;
  vector<short> rows;
  vector<short> cols;
  vector<short> planes;

  filename = Rcpp::as<string>(vfile).c_str();

  if(Rcpp::RObject(indicesOfVolumes).isNULL() == false)
    volumes = Rcpp::as<vector<short> >(indicesOfVolumes);

  if(Rcpp::RObject(indicesOfRows).isNULL() == false)
    rows = Rcpp::as<vector<short> >(indicesOfRows);

  if(Rcpp::RObject(indicesOfCols).isNULL() == false)
    cols = Rcpp::as<vector<short> >(indicesOfCols);

  if(Rcpp::RObject(indicesOfPlanes).isNULL() == false)
    planes = Rcpp::as<vector<short> >(indicesOfPlanes);

  CRECATFile inputFile(filename);

  if(inputFile.open(QIODevice::ReadOnly) &&
     (inputFile.format() != CECATFile::Undefined) )
  {
    Rcpp::List mhead;

    if(inputFile.readMainHeader_Rcpp(mhead) == true)
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
        cerr << "ERROR: can handle only a single dynamic index (one of  frame/gate/bed!" << endl;
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
        // we have to check if the index vector exceeds the boundaries
        vector<short>::iterator it;
        for(it = volumes.begin(); it < volumes.end(); ++it)
        {
          if( (*it < 1) || (*it > num_volumes))
          {
            cerr << "ERROR: index in volumes vector is exceeding boundaries "
                 << "there are " << num_volumes << " volumes in this file." << endl;
            return rList;
          }
        }
      }

      Rcpp::NumericVector frame_midpoint;
      Rcpp::StringVector volume_tags;

      vector<short>::iterator it;
      for(it = volumes.begin(); it != volumes.end(); ++it)
      {
        QByteArray* byteData = NULL;
        Rcpp::List subHeader;

        switch(which_to_use)
        {
          case FRAMES: frame = *it; break;
          case GATES: gate = *it; break;
          case BEDS: bed = *it - 1; break; // numbering of beds starts at 0
        }

        if(inputFile.readSubHeader_Rcpp(subHeader, frame, 1, gate, bed, 0) == true)
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
            // we have to check if the index vector exceeds the boundaries
            vector<short>::iterator it;
            for(it = rows.begin(); it < rows.end(); ++it)
            {
              if( (*it < 1) || (*it > y_dimension))
              {
                cerr << "ERROR: index in rows vector is out of bounds "
                     << "there are " << y_dimension << " rows in this file." << endl;
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
            // we have to check if the index vector exceeds the boundaries
            vector<short>::iterator it;
            for(it = cols.begin(); it < cols.end(); ++it)
            {
              if( (*it < 1) || (*it > x_dimension))
              {
                cerr << "ERROR: index in cols vector is out of bounds "
                     << "there are " << x_dimension << " cols in this file." << endl;
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
            // we have to check if the index vector exceeds the boundaries
            vector<short>::iterator it;
            for(it = planes.begin(); it < planes.end(); ++it)
            {
              if( (*it < 1) || (*it > z_dimension))
              {
                cerr << "ERROR: index in planes vector is out of bounds "
                     << "there are " << z_dimension << " planes in this file." << endl;
                exceeding_boundaries = true;
              }
            }
          }

          if(exceeding_boundaries == true)
            break;

          if(inputFile.readMatrix(byteData, frame, 1, gate, bed, 0) == true)
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

            QDataStream stream(byteData, QIODevice::ReadOnly);

            #if !defined(WORDS_BIGENDIAN)
            stream.setByteOrder(QDataStream::LittleEndian);
            #endif

            if(data_type == CECATSubHeader::SunShort)
            {
              int plane_size = x_dimension * y_dimension;

              // in cases were we only want some slices of the data
              // our destination matrix is smaller (equivalent to the length
              // of the index vectors)
              // and we have to flip x and y
              int destination_plane_size = cols.size() * rows.size();
              int destination_x_dimension= rows.size();

              // iterate through our index vectors
              // and copy only the relevant data
              vector<short>::iterator itPlane = planes.begin();
              for(int z = 0; itPlane < planes.end(); ++itPlane, ++z)
              {
                if(z != (*itPlane-1))
                  stream.device()->seek( (*itPlane-1) * plane_size * sizeof(qint16) );

                vector<short>::iterator itRow = rows.begin();
                for(int y = 0; itRow < rows.end(); ++itRow, ++y)
                {
                  if(y != (*itRow-1))
                    stream.device()->seek( ((*itPlane-1) * plane_size +
                                            (*itRow-1) * y_dimension) * sizeof(qint16) );

                  vector<short>::iterator itColumn = cols.begin();
                  for(int x = 0; itColumn < cols.end(); ++itColumn, ++x)
                  {
                    if(x != (*itColumn-1))
                      stream.device()->seek( ((*itPlane-1) * plane_size + (*itRow-1) * y_dimension +
                                              (*itColumn-1)) * sizeof(qint16) );

                    qint16 v;
                    stream >> v;

                    float voxel = static_cast<float>(v) * scale_factor * ecat_calibration_factor;

                    // calculate the index in the matrix data (excange x and y)
                    int index = x * destination_x_dimension + y + destination_plane_size * z;
                    matrixData[index] = voxel;
                  }
                }
              }
            }
            else
              cerr << "ERROR: currently only data_type 6 (SunShort) supported" << endl;

            QString tag(which_to_use);
            tag.append(QString::number(*it));
            volume_tags.push_back(tag.toAscii().constData());
            rList.push_back(matrixData, tag.toAscii().constData());

            // calculate the frame midpoint of this volume
            unsigned int frame_duration = Rcpp::as<unsigned int>(subHeader["frame_duration"]) / 1000; // [s]
            unsigned int frame_start = Rcpp::as<unsigned int>(subHeader["frame_start"]) / 1000; // [s]
            double fm = frame_start + frame_duration / 2;
            frame_midpoint.push_back(fm);
          }
          else
            cerr << "ERROR: can't read matrix data" << endl;
        }
        else
          cerr << "ERROR: can't read the sub header" << endl;

        delete byteData;
      }

      // set the volume names of the midpoints
      frame_midpoint.attr("names") = volume_tags;

      // create the attributes of the ecat structur
      rList.attr("class") = "Ecat";
      rList.attr("mhead") = mhead;
      rList.attr("frame_midpoint") = frame_midpoint;
    }
    else
      cerr << "ERROR: can't read main header" << endl;

    inputFile.close();
  }
  else
    cerr << "cannot open file '" << filename.toAscii().constData()
         << "': there is no such ecat file" << endl;

  return rList;

  END_RCPP
}

RcppExport SEXP writeEcat(SEXP ecat, SEXP vfilename)
{
  BEGIN_RCPP

  bool result = true;
  Rcpp::List RcppEcatFile;
  QString outputFileName;

  if(Rcpp::RObject(vfilename).isNULL())
  {
    cerr << "ERROR: no output file name given." << endl;
    result = false;
  }
  else
    outputFileName = Rcpp::as<string>(vfilename).c_str();

  if(Rcpp::RObject(ecat).isNULL())
  {
    cerr << "ERROR: no ecat data given." << endl;
    result = false;
  }
  else
  {
    RcppEcatFile = Rcpp::as<Rcpp::List>(ecat);
  }

  if(result == true)
  {
    if(QFileInfo(outputFileName).exists())
    {
      cerr << "ERROR: output file already exists." << endl;
      result = false;
    }
  }

  if(result == true)
  {
    CRECATFile outputFile(outputFileName, CECATMainHeader::ECAT7_Volume16);

    if(outputFile.open(QIODevice::ReadWrite) && outputFile.format() != CECATFile::Undefined)
    {
      Rcpp::List RcppMainHeader = RcppEcatFile.attr("mhead");
      //int file_type = Rcpp::as<int>(RcppMainHeader("file_type"));

      if(outputFile.writeMainHeader_Rcpp(RcppMainHeader) == false)
      {
        cerr << "ERROR: could not write destination main header." << endl;
        outputFile.remove();
        result = false;
      }

      // now we write the matrix data to the file
      // therefor we need to read out the names of the volumes
      // and check the first char of the name to know if frames, gates or beds
      // are used as volumes in the R ecat data structure ('f', 'g' or 'b')
      vector<string> names = Rcpp::as<vector<string> >(RcppEcatFile.names());
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
              cerr << "ERROR: there is something wrong with the volume names." << endl;
          }

          short x_dimension = Rcpp::as<short>(rSubHeader["x_dimension"]);
          short y_dimension = Rcpp::as<short>(rSubHeader["y_dimension"]);
          short z_dimension = Rcpp::as<short>(rSubHeader["z_dimension"]);

          float ecat_calibration_factor = Rcpp::as<float>(RcppMainHeader["ecat_calibration_factor"]);

          int plane_size = x_dimension * y_dimension;
          int dataSize = plane_size * z_dimension;

          // check if the dimensions are correct
          if(dataSize == volumeVector.size())
          {
            QByteArray* matrixData = new QByteArray(dataSize * sizeof(qint16), 0);

            QDataStream stream(matrixData, QIODevice::WriteOnly);
            #if !defined(WORDS_BIGENDIAN)
            stream.setByteOrder(QDataStream::LittleEndian);
            #endif

            float newMin = FLT_MAX;
            float newMax = -FLT_MAX;

            // search for the new min and max values
            for(int index = 0; index < dataSize; ++index)
            {
              volumeVector[index] /= ecat_calibration_factor;
              float voxel = volumeVector[index];

              newMin = qMin(newMin, voxel);
              newMax = qMax(newMax, voxel);
            }
          
            // calculate the scale factor
            float maxValue = qMax(qAbs(newMax), qAbs(newMin));
            float newScaleFactor = maxValue / static_cast<float>(SHRT_MAX);

            // now we scale and copy the image data   
            for(int z = 0; z < z_dimension; ++z)
            {
              for(int y = 0; y < y_dimension; ++y)
              {
                for(int x = 0; x < x_dimension; ++x)
                {
                  int index = x * x_dimension + y + plane_size * z;
                  float voxel = volumeVector[index] / newScaleFactor;

                  qint16 voxelInt16 = static_cast<qint16>(qRound(voxel));
                  stream << voxelInt16;
                }
              }
            }            

            rSubHeader["scale_factor"] = newScaleFactor;
            rSubHeader["image_min"] = static_cast<short>(qRound(newMin / newScaleFactor));
            rSubHeader["image_max"] = static_cast<short>(qRound(newMax / newScaleFactor));

            if(outputFile.writeSubHeader_Rcpp(rSubHeader, frame, 1, gate, bed))
            {
              if(outputFile.writeMatrix(*matrixData, frame, 1, gate, bed) == false)
              {
                cerr << "Error: writing matrix data failed for volume " << volume << endl;
                delete matrixData;
                outputFile.remove();
                result = false;
                break;
              }
            }
            else
            {
              cerr << "Error: writing subheader of volume " << volume << " failed." << endl;
              delete matrixData;
              outputFile.remove();
              result = false;
              break;
            }

            delete matrixData;
          }
          else
          {
            cerr << "Error in volume " << volume << ": the dimensions are not correct" << endl;
            outputFile.remove();
            result = false;
            break;
          }
        }
      }
      else
      {
        cerr << "ERROR: there is no volume data in the ecat data." << endl;
        outputFile.remove();
        result = false;
      }

      outputFile.close();
    }
    else
    {
      cerr << "ERROR: could not open destination file." << endl;
      result = false;
    }
  }

  return Rcpp::wrap(result);

  END_RCPP
}
