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

#ifndef CRECATFile_H
#define CRECATFile_H

#include <Rcpp.h>

#include <CECATFile>
#include <CECATMainHeader>

class CRECATFile : public CECATFile
{
  public:
    CRECATFile(const QString& filename,
               CECATMainHeader::Type fileType = CECATMainHeader::Unknown);
    CRECATFile(CECATMainHeader::Type fileType = CECATMainHeader::Unknown);

    // read methods
    bool readMainHeader(Rcpp::List& mainHeader);
    bool readSubHeader(Rcpp::List& subHeader,
                       short frame, short plane=1, short gate=1, short bed=0, short data=0);

    // write methods
    bool writeMainHeader(Rcpp::List& mainHeader);
    bool writeSubHeader(Rcpp::List& subHeader,
                        short frame, short plane=1, short gate=1, short bed=0, short data=0);

    CECATMainHeader* createMainHeader(Rcpp::List& rMainHeader);
    Rcpp::List createEmptyMainHeader(void);
    Rcpp::List createEmptySubHeader(void);
};
#endif
