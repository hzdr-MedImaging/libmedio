/* vim:set ts=2 nowrap: ****************************************************

 LMMC - ListMode Movement Correction
 Copyright (C) 2003 by Jens Langner <Jens.Langner@light-speed.de>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

#ifndef CECATDIRECTORYITEM_H
#define CECATDIRECTORYITEM_H

#include <qcstring.h>
#include <qdatastream.h>

// special macros to convert the MatrixID to their respect
// frame/plane/gate/bed and data representation
//
// This is based on the specification taken from the CTI ecat library
// source code
#define matrixID2Frame(x)	((x) & 0x1FF)
#define matrixID2Plane(x)	((((x)>>16) & 0xFF) + ((((x)>>9) & 0x3)<<8))
#define matrixID2Gate(x)	(((x)>>24) & 0x3F)
#define matrixID2Bed(x)		(((x)>>12)&0xF)
#define matrixID2Data(x)	((((x)>>9) & 0x4) | (((x)>>30) & 0x3))
#define frame2MatrixID(x)	((x) & 0x1FF)
#define plane2MatrixID(x)	((((x) & 0xFF)<<16) | ((((x) & 0x300)>>8)<<9))
#define gate2MatrixID(x)	(((x) & 0x3F)<<24)
#define bed2MatrixID(x)		(((x) & 0xF)<<12)
#define data2MatrixID(x)	((((x) & 0x3)<<30) | (((x) & 0x4)<<9))

#define convertToMatrixID(f, p, g, b, d)	( frame2MatrixID(f) | plane2MatrixID(p) | gate2MatrixID(g) | bed2MatrixID(b) | data2MatrixID(d) )

#include "CECATSubHeader.h"

// forward declarations
class CECATFile;

class CECATDirectoryItem
{
	public:
		enum AccessStatus { Deleted=-1, NotYetWritten=0, Finished=1 };

		CECATDirectoryItem(CECATSubHeader::Type subHeaderType);
	
		unsigned int getMatrixID(void)	{ return m_Entry.Matrix_ID;									}
		short frameID(void)							{ return matrixID2Frame(m_Entry.Matrix_ID); }
		short planeID(void)							{ return matrixID2Plane(m_Entry.Matrix_ID); }
		short gateID(void)							{ return matrixID2Gate(m_Entry.Matrix_ID);	}
		short bedID(void)								{ return matrixID2Bed(m_Entry.Matrix_ID);		}
		short dataID(void)							{	return matrixID2Data(m_Entry.Matrix_ID);	}
		CECATSubHeader* getSubHeader()	{ return m_pSubHeader;											}

		// accessor methods
		QByteArray* getMatrix(void);
		void* getMatrixData(void);

		// mutabor methods
		void setMatrix(QByteArray* pMatrix) { m_pMatrixData = pMatrix; }
		void setMatrixData(void* matrix, unsigned int size);

		// file i/o routines
		bool loadFromFile(CECATFile* pECATFile, long filePosition=0);
		bool saveToFile(CECATFile* pECATFile);

	protected:
		bool loadMatrixData();

	private:
		CECATFile*			m_pECATFile;		// pointer to the ECATFile from which we loaded the SubHeader.
		long						m_FilePosition;	// the position within the ECAT file
		CECATSubHeader*	m_pSubHeader;		// each item in a directory has a header
		QByteArray*			m_pMatrixData;	// pointer to the MatrixData (NULL if not available yet)

		// the matrix structure which we read out
		// of the ECAT file
		#pragma pack(2)
		struct
		{
			Q_UINT32 Matrix_ID;
			Q_UINT32 Matrix_SubHeader_StartPosition;
			Q_UINT32 Matrix_DataBlock_EndPosition;
			Q_UINT32 Matrix_Status;					// the access status of the item
		} m_Entry;
		#pragma pack()
};

#endif // CECATDIRECTORYITEM_H
