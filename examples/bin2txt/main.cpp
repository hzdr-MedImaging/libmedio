#include <stdio.h>
#include <iostream>
#include <iomanip>

#include <QFile>
#include <CPhilipsBinFile.h>

int main(int, char *argv[])
{
  CPhilipsBinFile file(argv[1]);
  if(file.open(QIODevice::ReadOnly))
  {
    char* data;
    unsigned int length = 0;

    std::cout << "magicNumber..: " << std::hex << file.header().magicNumber << std::endl;
    std::cout << "headerVersion: " << std::dec << file.header().headerVersion << std::endl;
    std::cout << "headerWords..: " << std::dec << file.header().headerWords << std::endl;
    std::cout << "dataType.....: " << std::dec << file.header().dataType << std::endl;
    std::cout << "zDim.........: " << std::dec << file.header().zDim << std::endl;
    std::cout << "yDim.........: " << std::dec << file.header().yDim << std::endl;
    std::cout << "xDim.........: " << std::dec << file.header().xDim << std::endl;

    if(file.readMatrix(data, length))
    {
      std::cout << "data length: " << length << std::endl;
      for(quint32 z=0; z < file.header().zDim; z++)
      {
        for(quint32 y=0; y < file.header().yDim; y++)
        {
          for(quint32 x=0; x < file.header().xDim; x++)
          {
            std::cout << "data[x:" << std::dec << x << ",y:" 
                                   << std::dec << y << ",z:"
                                   << std::dec << z << "]: " << std::fixed << std::setprecision(2) << static_cast<double>(data[y*z+x]) << std::endl;
          }
        }
      }

      delete[] data;
    }

    file.close();
  }

  return EXIT_SUCCESS;
}
