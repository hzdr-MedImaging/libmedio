//! @file CConcordeSinogram.h
//! @author Hagen Moelle
//! @date 11/13/2004
//! @brief contains definition of the class CConcordeSinogram

#ifndef CCONCORDESINOGRAM_H
#define CCONCORDESINOGRAM_H

#include "CConcordeFile.h"
#include "CMedIOData.h"

#include <QString>
//#include <qcstring.h>

//! @class CConcordeSinogram
//! @brief class which handles Concorde microPET sinograms
//! @bug no bugs reported
//! @warning still in development
//!
//! This class provides access to all kind of microPET sinograms.
//! 
class CConcordeSinogram : public CConcordeFile
{
	public :
	//contructors
		//! @brief constructor
		//! @param file: complete path to file holding concorde sinogram
		CConcordeSinogram(QString file);
		
	//destructor
		//! @brief destructor
		~CConcordeSinogram();
		
	//members
			
	//methods
		//! @brief access to data of a sinogram 
		//! @return array with data of specific frame in sinogram or NULL on error 
		bool readMatrix(QByteArray*& matrixData, short frame);
		
		//! @brief run time typeinformation  
		//! @return specific class type at runtime
		int fileType() {return CConcordeFile::ConcordeMicropet_Sinogram;}

		//! @brief loads a given file 
		//! @return true if loading is succesful otherwise false
		bool open(QIODevice::OpenModeFlag mode);
		
	private :
	//members
		
	//methods
};

#endif
