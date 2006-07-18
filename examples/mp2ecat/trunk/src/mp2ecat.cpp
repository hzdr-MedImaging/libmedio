//! @file main.cpp
//! @author Hagen Moelle
//! @date 11/13/2004

#include "CApplication.h"

#include <rtdebug.h>

using namespace std;

int main( int argc, char ** argv ) 
{
	ENTER();
	bool bResult = false;
	CApplication mp2Ecat;
	if(bResult = mp2Ecat.parseCmdLine(argc, argv))
	{
			bResult = mp2Ecat.convertFile();
	}
	if(bResult)
	{
		RETURN(0);
		return 0;
	}
	else
	{
		RETURN(1);
		return 1;
	}
}
