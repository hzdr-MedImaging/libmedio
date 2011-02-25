//! @file main.cpp
//! @author Hagen Moelle
//! @date 11/13/2004

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
  CRTDebug::instance()->init("mp2ecat");
  #endif

  ENTER();

  // instantiate the sorter application
	CApplication mp2Ecat;
	bool bResult = mp2Ecat.parseCmdLine(argc, argv);
	if(bResult)
		bResult = mp2Ecat.convertFile();

	if(bResult)
    returnCode = EXIT_SUCCESS;
	else
	  returnCode = EXIT_FAILURE;

  #if defined(DEBUG)
  CRTDebug::destroy();
  #endif

  return returnCode;
}
