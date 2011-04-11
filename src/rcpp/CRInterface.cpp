#include "CRInterface.h"

#include <iostream>
#include <Rcpp.h>

#include <CECATFile.h>
#include <CECAT7MainHeader.h>
#include <CECAT7SubHeaderImage.h>

using namespace std;

RcppExport SEXP readEcat(SEXP vfile,
                         SEXP numberOfVolumes,
                         SEXP numberOfRows,
                         SEXP numberOfCols,
                         SEXP numberOfPlanes)
{
  Rcpp::List rList;

  QString filename;
  vector<short> nv;
  vector<short> rows;
  vector<short> cols;
  vector<short> planes;

  try
  {
    QString filename = Rcpp::as<string>(vfile).c_str();

    if(Rcpp::RObject(numberOfVolumes).isNULL() == false)
      nv = Rcpp::as<vector<short> >(numberOfVolumes);

    if(Rcpp::RObject(numberOfRows).isNULL() == false)
      rows = Rcpp::as<vector<short> >(numberOfRows);

    if(Rcpp::RObject(numberOfCols).isNULL() == false)
      cols = Rcpp::as<vector<short> >(numberOfCols);

    if(Rcpp::RObject(numberOfPlanes).isNULL() == false)
      planes = Rcpp::as<vector<short> >(numberOfPlanes);

    cout << "nv.size() = " << nv.size() << endl;

    CRInterface inputFile(filename);

    if(inputFile.open(QIODevice::ReadOnly) &&
       (inputFile.format() != CECATFile::Undefined) )
    {
      Rcpp::List mhead;
      Rcpp::List subHeader;

      cout << "lenght of rows: " << rows.size() << endl;

      if(inputFile.readMainHeader(mhead) == true)
      {
        int num_frames = static_cast<int>(mhead("num_frames"));
        int num_gates = static_cast<int>(mhead("num_gates"));
        int num_bed_pos = static_cast<int>(mhead("num_bed_pos")) + 1;
        int num_volumes = 0;

        if((num_frames >= 1) && (num_gates <= 1) && (num_bed_pos <= 1))
        {
          num_volumes = num_frames;
        }

        Rcpp::NumericVector* matrixData = NULL;

        for(short frame = 1; frame <= num_frames; frame++)
        {

          if(inputFile.readMatrix(matrixData, subHeader, frame/*, 1, gate, bed, data*/) == true)
          {
            QString name("f");
            name.append(QString::number(frame));

            if(rows.size() != 0)
            {
              vector<short>::iterator it;
              for(it = rows.begin(); it != rows.end(); ++it)
                cout <<  *it << endl;
            }

            rList.push_back(*matrixData, name.toAscii().constData());

            rList.attr("class") = "Ecat";
            rList.attr("mhead") = mhead;

            Rcpp::List::iterator it = rList.begin();
            Rcpp::NumericVector frame_midpoint;
            for(; it != rList.end(); it++)
            {
              unsigned int frame_duration = static_cast<unsigned int>(subHeader("frame_duration"));
              unsigned int frame_start = static_cast<unsigned int>(subHeader("frame_start"));
              double fm = frame_start + frame_duration / 2;
              frame_midpoint.push_back(fm);
            }

            rList.attr("frame_midpoint") = frame_midpoint;
          }
          else
          {
            cerr << "ERROR: can't read matrix data" << endl;
          }

          if(matrixData != NULL)
            delete matrixData;
        }

      }
      else
      {
        cerr << "ERROR: can't read main header" << endl;
      }
    }
    else
    {
      cerr << "cannot open file '" << filename.toAscii().constData()
           << "': there is no such ecat file" << endl;
    }
  }
  catch(Rcpp::not_compatible e)
  {
    forward_exception_to_r(e);
  }
 
  return rList;
}

CRInterface::CRInterface(const QString& filename, CECATMainHeader::Type fileType )
  : CECATFile(filename, fileType)
{
}

bool CRInterface::readMainHeader(Rcpp::List& mainHeader)
{
  bool bResult = true;

  CECATMainHeader* cppMainHeader = NULL;
  if(CECATFile::readMainHeader(cppMainHeader) == true)
  {
    if(format() == CECATFile::ECAT7)
    {
      CECAT7MainHeader *mHeader = static_cast<CECAT7MainHeader*>(cppMainHeader);
      
      QString magic_number = mHeader->magic_Number();
      int file_type = mHeader->file_Type();
      Rcpp::Datetime scan_start_time = Rcpp::Datetime(mHeader->scan_Start_Time());
      float ecat_calibration_factor = mHeader->calibration_Factor();
      int calibration_units = mHeader->calibration_Units();
      int num_frames = mHeader->num_Frames();
      int num_gates = mHeader->num_Gates();
      int num_bed_pos = mHeader->num_Bed_Pos();
      string data_units = mHeader->data_Units();

      mainHeader = Rcpp::List::create(Rcpp::Named("magic_number") = magic_number.toAscii().constData(),
                                      Rcpp::Named("file_type")    = file_type,
                                      Rcpp::Named("scan_start_time") = scan_start_time,
                                      Rcpp::Named("ecat_calibration_factor") = ecat_calibration_factor,
                                      Rcpp::Named("calibration_units") = calibration_units,
                                      Rcpp::Named("num_frames") = num_frames,
                                      Rcpp::Named("num_gates") = num_gates,
                                      Rcpp::Named("num_bed_pos") = num_bed_pos,
                                      Rcpp::Named("data_units") = data_units);
    }
    else
    {
      cerr << "ERROR: provided input file is not an EACAT7 file." << endl;
      bResult = false;
    }

    delete cppMainHeader;
  }
  else
  {
    bResult = false;
  }

  return bResult;
}

bool CRInterface::readMatrix(Rcpp::NumericVector*& matrixData, Rcpp::List& subHeader,
                            short frame, short plane, short gate, short bed, short data)
{
  bool bResult = true;

  CECATSubHeader* sHead = NULL;
  QByteArray* byteData = NULL;

  if(CECATFile::readMatrix(byteData, sHead, frame, plane, gate, bed, data) == true)
  {
    CECAT7SubHeaderImage* sHeadImage = static_cast<CECAT7SubHeaderImage*>(sHead);
    int data_type = sHeadImage->data_Type();
    short x_dimension = sHeadImage->x_Dimension();
    short y_dimension = sHeadImage->y_Dimension();
    short z_dimension = sHeadImage->z_Dimension();
    float scale_factor = sHeadImage->scale_Factor();
    float x_pixelsize = sHeadImage->x_Pixel_Size();
    float y_pixelsize = sHeadImage->y_Pixel_Size();
    float z_pixelsize = sHeadImage->z_Pixel_Size();
    unsigned int frame_duration = sHeadImage->frame_Duration() / 1000; // [s]
    unsigned int frame_start = sHeadImage->frame_Start_Time() / 1000;  // [s]

    short minRow = 1;
    short maxRow = y_dimension;
    short minCol = 1;
    short maxCol = x_dimension;
    short minPlane = 1;
    short maxPlane = z_dimension;

    short rowCount = maxRow - minRow + 1;
    short colCount = maxCol - minCol + 1;
    short planeCount = maxPlane - minPlane + 1;

    Rcpp::IntegerVector rows(rowCount);
    Rcpp::IntegerVector cols(colCount);
    Rcpp::IntegerVector planes(planeCount);

    for(int i = minRow; i <= maxRow; i++)
      rows[i-minRow] = i;

    for(int i = minCol; i <= maxCol; i++)
      cols[i-minCol] = i;

    for(int i = minPlane; i <= maxPlane; i++)
      planes[i-minPlane] = i;

    subHeader = Rcpp::List::create(Rcpp::Named("data_type") = data_type,
                                   Rcpp::Named("x_dimension") = x_dimension,
                                   Rcpp::Named("y_dimension") = y_dimension,
                                   Rcpp::Named("z_dimension") = z_dimension,
                                   Rcpp::Named("scale_factor") = scale_factor,
                                   Rcpp::Named("x_pixelsize") = x_pixelsize,
                                   Rcpp::Named("y_pixelsize") = y_pixelsize,
                                   Rcpp::Named("z_pixelsize") = z_pixelsize,
                                   Rcpp::Named("frame_duration") = frame_duration,
                                   Rcpp::Named("frame_start") = frame_start);

    Rcpp::Dimension dim(y_dimension, x_dimension, z_dimension);

    matrixData = new Rcpp::NumericVector(dim);

    matrixData->attr("class") = Rcpp::StringVector::create("Volume", "array");
    matrixData->attr("shead") = subHeader;
    matrixData->attr("voxsiz") = Rcpp::NumericVector::create(y_pixelsize, x_pixelsize, z_pixelsize);
    matrixData->attr("vgrid") = Rcpp::List::create(Rcpp::Named("rows") = rows,
                                                   Rcpp::Named("cols") = cols,
                                                   Rcpp::Named("planes") = planes);

    QDataStream stream(byteData, QIODevice::ReadOnly);

#if !defined(WORDS_BIGENDIAN)
    stream.setByteOrder(QDataStream::LittleEndian);
#endif

    int planeSize = rowCount*colCount;

    if (data_type == CECATSubHeader::SunShort)
    {
      for(int z = 0; z < planeCount; z++)
        for(int y = 0; y < rowCount; y++)
          for(int x = 0; x < colCount; x++)
          {
            long index = x * colCount + y + planeSize * z;
            qint16 v;
            stream >> v;

            matrixData->operator[](index) = static_cast<double>(v)*scale_factor;
          }
    }
    else
    {
        cerr << "ERROR: currently only data_type 6 (SunShort) supported" << endl;
        bResult = false;
    }
  }
  else
  {
    bResult = false;
  }

  if(byteData != NULL)
    delete byteData;

  if(sHead != NULL)
    delete sHead;

  return bResult;
}
