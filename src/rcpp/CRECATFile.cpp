#include "CRECATFile.h"

#include <iostream>
#include <Rcpp.h>

#include <CECATFile.h>
#include <CECAT7MainHeader.h>
#include <CECAT7SubHeaderImage.h>

using namespace std;

CRECATFile::CRECATFile(const QString& filename, CECATMainHeader::Type fileType )
  : CECATFile(filename, fileType)
{
}

bool CRECATFile::readMainHeader_Rcpp(Rcpp::List& mainHeader)
{
  bool result = false;

  CECATMainHeader* cppMainHeader = NULL;
  if(readMainHeader(cppMainHeader) == true)
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
      result = true;
    }
    else
      cerr << "ERROR: provided input file is not an EACAT7 file." << endl;

    delete cppMainHeader;
  }

  return result;
}

bool CRECATFile::readSubHeader_Rcpp(Rcpp::List& subHeader,
                                    short frame, short plane, short gate, short bed, short data)
{
  bool result = false;
  CECATSubHeader* sHead = NULL;

  if(readSubHeader(sHead, frame, plane, gate, bed, data) == true)
  {
    if(format() == CECATFile::ECAT7)
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

      result = true;
    }
    else
      cerr << "ERROR: provided input file is not an EACAT7 file." << endl;

    delete sHead;
  }

  return result;
}
