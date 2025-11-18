/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2025 hzdr.de and contributors
 
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
    http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/


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
