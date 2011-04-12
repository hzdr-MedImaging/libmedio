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

    cout << "nv.size() = " << nv.size() << endl;

    CRECATFile inputFile(filename);

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
