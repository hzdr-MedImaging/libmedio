#include "RInterface.h"
#include "CRECATFile.h"

#include <iostream>
#include <Rcpp.h>

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

    CRECATFile inputFile(filename);

    if(inputFile.open(QIODevice::ReadOnly) &&
       (inputFile.format() != CECATFile::Undefined) )
    {
      Rcpp::List mhead;
      Rcpp::List subHeader;

      if(inputFile.readMainHeader(mhead) == true)
      {
        int num_frames = static_cast<int>(mhead("num_frames"));
        int num_gates = static_cast<int>(mhead("num_gates"));
        int num_bed_pos = static_cast<int>(mhead("num_bed_pos")) + 1;
        int num_volumes = 0;

        Rcpp::NumericVector* matrixData = NULL;
        Rcpp::NumericVector frame_midpoint;
        Rcpp::StringVector volume_tags;

        for(short frame = 1; frame <= num_frames; frame++)
        {

          if(inputFile.readMatrix(matrixData, subHeader, frame/*, 1, gate, bed, data*/) == true)
          {
            QString tag("f");
            tag.append(QString::number(frame));

            volume_tags.push_back(tag.toAscii().constData());
            rList.push_back(*matrixData, tag.toAscii().constData());

            // calculate the frame midpoint
            unsigned int frame_duration = static_cast<unsigned int>(subHeader("frame_duration"));
            unsigned int frame_start = static_cast<unsigned int>(subHeader("frame_start"));
            double fm = frame_start + frame_duration / 2;
            frame_midpoint.push_back(fm);
          }
          else
             cerr << "ERROR: can't read matrix data" << endl;
 
          if(matrixData != NULL)
            delete matrixData;
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
    }
    else
      cerr << "cannot open file '" << filename.toAscii().constData()
           << "': there is no such ecat file" << endl;
  }
  catch(Rcpp::not_compatible e)
  {
    forward_exception_to_r(e);
  }

  return rList;
}
