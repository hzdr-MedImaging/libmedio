/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004 by Jens Langner <Jens.Langner@light-speed.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

#ifndef CECATDIRECTORYITEM_H
#define CECATDIRECTORYITEM_H

#include <qcstring.h>
#include <qdatastream.h>

// some useful defines on the ECAT file format
// especially for ECATBlock calculation as ECAT Files
// are seperated in 512 byte blocks and positions within
// the directorylist are refered in this format starting
// from 1.
#define ECAT_BLOCKSIZE				512
#define ECAT_POS_MAINHEADER		1		// the MainHeader is always at block 1
#define ECAT_POS_MAINDIR			2		// the MainDir is always at block 2
#define ECATBlock2FilePos(v)	(((v)-1)*ECAT_BLOCKSIZE)
#define FilePos2ECATBlock(v)	((v)/ECAT_BLOCKSIZE+1)

// special macros to convert the MatrixID to their respect
// frame/plane/gate/bed and data representation
#define matrixID2Frame(x)	((x) & 0x1FF)
#define matrixID2Plane(x)	((((x)>>16) & 0xFF) + ((((x)>>9) & 0x3)<<8))
#define matrixID2Gate(x)	(((x)>>24) & 0x3F)
#define matrixID2Bed(x)		(((x)>>12) & 0xF)
#define matrixID2Data(x)	((((x)>>9) & 0x4) | (((x)>>30) & 0x3))
#define frame2MatrixID(x)	((x) & 0x1FF)
#define plane2MatrixID(x)	((((x) & 0xFF)<<16) | ((((x) & 0x300)>>8)<<9))
#define gate2MatrixID(x)	(((x) & 0x3F)<<24)
#define bed2MatrixID(x)		(((x) & 0xF)<<12)
#define data2MatrixID(x)	((((x) & 0x3)<<30) | (((x) & 0x4)<<9))

#define convertToMatrixID(f, p, g, b, d) ( frame2MatrixID(f) | \
																					 plane2MatrixID(p) | \
																					 gate2MatrixID(g)  | \
																					 bed2MatrixID(b)	 | \
																					 data2MatrixID(d) )

#include <CECATSubHeader.h>

// forward declarations
class CECATFile;

class Q_EXPORT CECATDirectoryItem
{
	public:
		enum AccessStatus { Deleted=-1, 
												NotYetWritten=0, 
												Finished=1
											};

		CECATDirectoryItem(CECATFile* file,
											 CECATSubHeader::Type subHeaderType = CECATSubHeader::Unknown,
											 Q_UINT32 matrixID = 0);
	
		// accessor methods
		unsigned int matrixID(void) const
		{ return convertToMatrixID(m_iFrame, m_iPlane, m_iGate, m_iBed, m_iData); }

		QIODevice::Offset dataBlock_Start(void) const
		{ return m_iDataBlock_Start; }
		
		QIODevice::Offset dataBlock_End(void) const
		{ return m_iDataBlock_End; }

		short frame(void) const
		{ return m_iFrame; }

		short plane(void) const
		{ return m_iPlane; }
		
		short gate(void) const
		{ return m_iGate; }
		
		short bed(void) const
		{ return m_iBed; }
		
		short data(void) const
		{	return m_iData; }

		// mutator methods
		void setDataBlock_Start(const QIODevice::Offset offset)
		{ m_iDataBlock_Start = offset; }

		void setDataBlock_End(const QIODevice::Offset offset)
		{ m_iDataBlock_End = offset; }
		
		void setFrame(const short f)
		{ m_iFrame = f; }

		void setPlane(const short p)
		{ m_iPlane = p; }
		
		void setGate(const short g)
		{ m_iGate = g; }
		
		void setBed(const short b)
		{ m_iBed = b; }
		
		void setData(const short d)
		{	m_iData = d; }		
			
		// read i/o methods
		bool readSubHeader(CECATSubHeader*& subHeader);
		bool readMatrix(QByteArray*& data);
		bool readMatrix(char*& data, unsigned int& len);

		// write i/o methods
		bool writeSubHeader(const CECATSubHeader& subHeader);
		bool writeMatrix(const QByteArray& data);
		bool writeMatrix(const char* data, unsigned int len);
		bool writeMatrix(const QByteArray& data, CECATSubHeader::Data_Type type);
		bool writeMatrix(const char* data, unsigned int len, CECATSubHeader::Data_Type type);

		// our QDataStream operators
		friend QDataStream& operator<<(QDataStream& stream, const CECATDirectoryItem& item);
		friend QDataStream& operator>>(QDataStream& stream, CECATDirectoryItem& item);

	private:
		CECATFile*						m_pECATFile;				// pointer to the ECATFile
		CECATSubHeader::Type	m_iSubHeaderType;		// the choosen subHeaderType for this item
		CECATSubHeader*				m_pCachedSubHeader;	// pointer to a cached SubHeader object

		// META information about the directory Item
		short									m_iFrame;						// information normally
		short									m_iPlane;						// encoded in
		short									m_iGate;						// the MatrixID
		short									m_iBed;
		short									m_iData;
		QIODevice::Offset			m_iDataBlock_Start; // also start of SubHeader
		QIODevice::Offset			m_iDataBlock_End;		// end of raw Data area
		enum AccessStatus			m_iStatus;
};

#endif // CECATDIRECTORYITEM_H
