#include "RInterface.h"

#include <iostream>
#include <Rcpp.h>

#include "CRECATFile.h"

using namespace std;

RcppExport SEXP readEcat(SEXP vfile,
                         SEXP indicesOfVolumes,
                         SEXP indicesOfRows,
                         SEXP indicesOfCols,
                         SEXP indicesOfPlanes)
{
  Rcpp::List rList;

  QString filename;
  vector<short> volumes;
  vector<short> rows;
  vector<short> cols;
  vector<short> planes;

  try
  {
    filename = Rcpp::as<string>(vfile).c_str();

    if(Rcpp::RObject(indicesOfVolumes).isNULL() == false)
      volumes = Rcpp::as<vector<short> >(indicesOfVolumes);

    if(Rcpp::RObject(indicesOfRows).isNULL() == false)
      rows = Rcpp::as<vector<short> >(indicesOfRows);

    if(Rcpp::RObject(indicesOfCols).isNULL() == false)
      cols = Rcpp::as<vector<short> >(indicesOfCols);

    if(Rcpp::RObject(indicesOfPlanes).isNULL() == false)
      planes = Rcpp::as<vector<short> >(indicesOfPlanes);
  }
  catch(Rcpp::not_compatible e)
  {
    forward_exception_to_r(e);
    // return the empty list
    return rList;
  }

  CRECATFile inputFile(filename);

  if(inputFile.open(QIODevice::ReadOnly) &&
     (inputFile.format() != CECATFile::Undefined) )
  {
    Rcpp::List mhead;

    if(inputFile.readMainHeader_Rcpp(mhead) == true)
    {
      int num_frames = static_cast<int>(mhead("num_frames"));
      int num_gates = static_cast<int>(mhead("num_gates"));
      int num_bed_pos = static_cast<int>(mhead("num_bed_pos"));
      float ecat_calibration_factor = static_cast<float>(mhead("ecat_calibration_factor"));

      enum use_as_volume
      {
        FRAMES = 'f',
        GATES = 'g',
        BEDS = 'b'
      } which_to_use;

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
        num_volumes = num_bed_pos;
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

      Rcpp::NumericVector frame_midpoint;
      Rcpp::StringVector volume_tags;

      vector<short>::iterator it;
      for(it = volumes.begin(); it != volumes.end(); ++it)
      {
        QByteArray* byteData = NULL;
        Rcpp::List subHeader;
       
        switch(which_to_use)
        {
          case FRAMES:
          {
            frame = *it;
            break;
          }
          case GATES:
          {
            gate = *it;
            break;
          }
          case BEDS:
          {
            bed = *it;
            break;
          }
        }

        if(inputFile.readSubHeader_Rcpp(subHeader, frame, 1, gate, bed, 0) == true)
        {
          int data_type = static_cast<int>(subHeader("data_type"));
          short x_dimension = static_cast<short>(subHeader("x_dimension"));
          short y_dimension = static_cast<short>(subHeader("y_dimension"));
          short z_dimension = static_cast<short>(subHeader("z_dimension"));
          float scale_factor = static_cast<float>(subHeader("scale_factor"));
          float x_pixelsize = static_cast<float>(subHeader("x_pixelsize"));
          float y_pixelsize = static_cast<float>(subHeader("y_pixelsize"));
          float z_pixelsize = static_cast<float>(subHeader("z_pixelsize"));

          // check if rows, cols and/or planes index arrays are given
          // if not take all of them
          if(rows.size() == 0)
          {
            for(int i = 1; i <= y_dimension; i++)
              rows.push_back(i);
          }

          if(cols.size() == 0)
          {
            for(int i = 1; i <= x_dimension; i++)
              cols.push_back(i);
          }

          if(planes.size() == 0)
          {
            for(int i = 1; i <= z_dimension; i++)
              planes.push_back(i);
          }

          if(inputFile.readMatrix(byteData, frame, 1, gate, bed, 0) == true)
          {
            Rcpp::Dimension dim(y_dimension, x_dimension, z_dimension);
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

            if (data_type == CECATSubHeader::SunShort)
            {
              int planeSize = x_dimension * y_dimension;

              for(int z = 0; z < z_dimension; z++)
                for(int y = 0; y < y_dimension; y++)
                  for(int x = 0; x < x_dimension; x++)
                  {
                    int index = x * x_dimension + y + planeSize * z;
                    qint16 v;
                    stream >> v;
                    double voxel = static_cast<double>(v) * scale_factor * ecat_calibration_factor;
                    matrixData[index] = voxel;
                  }
            }
            else
              cerr << "ERROR: currently only data_type 6 (SunShort) supported" << endl;

            QString tag(which_to_use);
            tag.append(QString::number(*it));
            volume_tags.push_back(tag.toAscii().constData());
            rList.push_back(matrixData, tag.toAscii().constData());

            // calculate the frame midpoint of this volume
            unsigned int frame_duration = static_cast<unsigned int>(subHeader("frame_duration"));
            unsigned int frame_start = static_cast<unsigned int>(subHeader("frame_start"));
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
}
