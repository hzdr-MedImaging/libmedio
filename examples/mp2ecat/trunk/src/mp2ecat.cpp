//! @file main.cpp
//! @author Hagen Moelle
//! @date 11/13/2004

#include "CCmdLineStart.h"

using namespace std;

int main( int argc, char ** argv ) 
{
	CCmdLineStart commandLine;
	commandLine.process(argc, argv);
}
