//! @file CMedIOHeader.h
//! @author Hagen Moelle
//! @date 11/13/2004
//! @brief contains definition of the class CMedIOHeader

#ifndef CMEDIOHEADER_H
#define CMEDIOHEADER_H

#include <qstring.h>

//! @class CMedIOHeader
//! @brief an abstract class which provides an interface for header information of medical data
//! @bug no bugs reported
//! @warning still in development
//!
//! This class is an interface for easier handling of different medical data headers.
//! All headers of data formats which are included in Medical IO Library should use the class
//! as a baseclass.
class CMedIOHeader
{
	public :
	//contructors
		//contructors
		//! @brief constructor
		//! @param file: complete path to file holding header of medical data
		CMedIOHeader(QString file);
		//contructors
		//! @brief default constructor
		CMedIOHeader(){}

	//destructor
		//! @brief destructor
		virtual ~CMedIOHeader();
		
	//members
		enum Format {ConcordeMicropet=0	//!< header of concorde microPET scanner
				};
		
	//methods
		//! @brief loads a given file 
		//! @return true if loading is succesful otherwise false
		virtual bool load() = 0;
		
		//! @brief saves the data  
		//! @return true if saving is succesful otherwise false
		virtual bool save() = 0;

		//! @brief run time typeinformation  
		//! @return specific class type at runtime
		virtual int rtti() = 0;
		
		//! @brief access to total number of frames in dataset  
		//! @return number of frames in dataset
		virtual int numframes(void) = 0;

		//! @brief sets number of frames in dataset  
		//! @param value: number of frames to be set
		virtual void setNumFrames(int value) = 0;
		
		//! @brief access to filename of header
		//! @return filename of medical data header file
		QString file() {return File;}
		
	private :
	//members
		QString File; //!< filename of medical data header file

	//methods
};

#endif
