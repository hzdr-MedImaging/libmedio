#ifndef CPHILIPSBINFILE_H
#define CPHILIPSBINFILE_H

#include "CMedIOData.h"

/*
struct PhilipsBinHeader2
{
    quint32 magic;         // Identifies this as a bin file
    qint32 version;
    qint32 headerWords;    // Header size, in 4-byte words
    qint32 type;           //Element data type
    qint32 nDims;
    qint32 *sizes;
    qint32 *starts;
};
*/

#pragma pack(1)
struct PhilipsBinHeader1
{
  quint32 magicNumber;   // Identifies this as a bin file
  quint32 headerVersion;
  quint32 headerWords;   // Header size, in 4-byte words
  quint32 dataType;
  quint32 zDim;          // zDim
  quint32 yDim;          // yDim
  quint32 xDim;          // xDim
};
#pragma pack()

class CPhilipsBinFile : public CMedIOData
{
  public:
    enum Data_Type
    {
      UnsignedInt8 = 0,
      SignedInt8 = 1,
      UnsignedInt16 = 2,
      SignedInt16 = 3,
      UnsignedInt32 = 4,
      SignedInt32 = 5,
      Float32 = 7,
      Float64 = 9
    };

    CPhilipsBinFile(QString filename);

    bool open(OpenModeFlag mode);
    quint32 readVersion();
    void close();

  CMedIOData::Format dataFormat() const {return CMedIOData::PhilipsBin;}

    // the matrix has the following data order: data[z][y][x]
    bool readMatrix(char*& data, unsigned int &length);

    PhilipsBinHeader1 header() const {return m_header;}

  private:
    PhilipsBinHeader1 m_header;

    void swap(char*& data);
    unsigned int elementSize() const;
    unsigned int numberOfElements() const;
};

#endif // CPHILIPSBINFILE_H
