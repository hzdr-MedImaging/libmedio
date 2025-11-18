/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2025 hzdr.de and contributors
 
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

#include "CApplication.h"

#include <rtdebug.h>
#include <cstdlib>

int main(int argc, char* argv[])
{
  int returnCode = EXIT_SUCCESS; // return no error on default

  // You want this, unless you mix couts output with C output.
  // Read  http://gcc.gnu.org/onlinedocs/libstdc++/27_io/howto.html#8 for an explanation.
  //std::ios::sync_with_stdio(false);

  // before we start anything serious, we need to initialize our
  // debug class
  #if defined(DEBUG)
  CRTDebug::instance()->init("img2ecat");
  #endif

  ENTER();

  // instantiate the sorter application
  CApplication img2Ecat;
  bool bResult = img2Ecat.parseCmdLine(argc, argv);
  if(bResult)
    bResult = img2Ecat.process();

  if(bResult)
    returnCode = EXIT_SUCCESS;
  else
    returnCode = EXIT_FAILURE;

  RETURN(returnCode);
  #if defined(DEBUG)
  CRTDebug::destroy();
  #endif

  return returnCode;
}
