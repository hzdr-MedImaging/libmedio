//! @file CCmdLineStart.h
//! @author Hagen Moelle
//! @date 11/13/2004
//! @brief contains definition of the class CCmdLineStart

#ifndef CCMDLINESTART_H
#define CCMDLINESTART_H

#include "CGetOpt.h"

#include <QString>

//! @class CCmdLineStart
//! @brief class which handles the commandline options 
//! @bug no bugs reported
//! @warning still in development
//!
//! This class will parse the commandline for parameters and
//! start multiple reconstructions if parameters are right.
//!
class CCmdLineStart
{
	public:
	//constructors
		//! @brief constructor
		CCmdLineStart();
		
	//destructor
		//! @brief destructor
		~CCmdLineStart(){ if(m_pGetOpt) delete m_pGetOpt;}

	//members

	//methods
		//! @brief processes command line
		//! @return error message
		QString process(int argc, char* argv[]);

	protected:
		//! @brief parses the commandline
		//! @param argc: number of parameters
		//! @param argv: list of parameters
		//! @return true if parsing is successful
		bool parse(int argc, char* argv[]);

		bool convertFile();

		// methods to get options out of the commandline
		QString setInputFileName(bool mustExist = true);
		QString setOutputFileName(bool mustExist = true);
		QString setPatientName(bool mustExist = false);
		
	private:
		CGetOpt* m_pGetOpt;	//!< commandline options object
		QString m_sInputFileName;
		QString m_sOutputFileName;
		QString m_sPatientName;
};

#endif // !CCMDLINESTART_H
