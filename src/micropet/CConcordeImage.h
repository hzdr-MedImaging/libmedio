//! @file CConcordeImage.h
//! @author Hagen Moelle
//! @date 11/13/2004
//! @brief contains definition of the class CConcordeImage

#ifndef CCONCORDEIMAGE_H
#define CCONCORDEIMAGE_H

#include "CConcordeFile.h"
#include "CMedIOData.h"

#include <qstring.h>
#include <qcstring.h>

//! @class CConcordeImage
//! @brief class which handles Concorde microPET imagevolumes
//! @bug no bugs reported
//! @warning still in development
//!
//! This class provides access to all kind of microPET imagevolumes.
//! 
class CConcordeImage : public CConcordeFile
{
	public :
	//contructors
		//! @brief constructor
		//! @param file: complete path to file holding concorde imagevolume
		CConcordeImage(QString file);
		
	//destructor
		//! @brief destructor
		~CConcordeImage();
		
	//members
		
	//methods
		//! @brief access to data of an imagevolume 
		//! @return array with data of specific frame in imagevolume or NULL on error
		QByteArray* getMatrix(short frame, short plane, 
			short gate, short bed, short data);
		
		//! @brief run time typeinformation  
		//! @return specific class type at runtime
		int fileType() {return CConcordeFile::ConcordeMicropet_Image;}
		
	private :
	//members
		
	//methods
};

#endif
