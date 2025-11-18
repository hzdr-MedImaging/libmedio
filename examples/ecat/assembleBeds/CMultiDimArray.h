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

#ifndef CMULTIDIMARRAY_H
#define CMULTIDIMARRAY_H

// System includes
#include <stdarg.h>
#include <stddef.h>
#include <exception>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <stdlib.h>

using namespace std;

template<class T, int D>
class CMultiDimArray
{
public:
  CMultiDimArray(int iStarts[D], int iLengths[D])
    :m_size(0),
     m_pData(NULL),
     m_pBase(NULL) 
  {
    for (int i = 0; i < D; i++)
    {
      m_iStarts[i] = iStarts[i];
      m_iLengths[i] = iLengths[i];
    }

    create();
  };

  CMultiDimArray(int iLengths[D])
    :m_size(0),
     m_pData(NULL),
     m_pBase(NULL)
  {
    for (int i = 0; i < D; i++)
    {  
      m_iStarts[i] = 0;
      m_iLengths[i] = iLengths[i];
    }

    create();
  };

  CMultiDimArray(const CMultiDimArray<T, D>&other)
    :m_size(0),
     m_pData(NULL),
     m_pBase(NULL)
  {
    deepCopy(other);
  };

  ~CMultiDimArray()
  {
    destroy();
  };

  void deepCopy(const CMultiDimArray<T, D>&other)
  {
    // Delete old memory
    destroy();
    // Copy the size paramter
    for (int i = 0; i < D; i++)
    {
      m_iStarts[i] = other.m_iStarts[i];
      m_iLengths[i] = other.m_iLengths[i];
    }
    // Allocate new memory and 
    // set the pointer according
    create();

    // Copy the data part
    memcpy(m_pBase, other.m_pBase, size() * sizeof(T));
  };
  void copyData(const CMultiDimArray<T, D>&other)
  {
    // NOTE: the user is responsible for the
    // correct size of both arrays
    // Copy the data part
    memcpy(m_pBase, other.m_pBase, size() * sizeof(T));
  };
  void operator=(const CMultiDimArray<T, D>&other)
  {
    deepCopy(other);  
  };

  T* rawData() const
  {
    // Return the pointer to the 
    // raw data, which means the
    // data without the pointers
    return reinterpret_cast<T*>(m_pBase);
  };

  size_t size() const 
  {
    return m_size;
  };

  const int* starts() const
  {
    return m_iStarts;
  };

  const int* lengths() const
  {
    return m_iLengths;
  };

  void clear()
  {
    memset(m_pBase, 0, size() * sizeof(T));
  };

  void fill(const T value)
  {
    // Copy the base pointer
    T* pTempBase = reinterpret_cast<T*>(m_pBase);
    // Initialize the data with
    // the given value
    for (size_t i = 0; i < size(); i++)
    {
      // Fill the current pointer pos
      // with the value
      *pTempBase++ = value;
    }
  };

  void setBases(const int iStarts[D])
  {
    size_t dimIndizes[D];
    // Set the new starts
    // and initialize dimIndizes
    // with 0
    for(int a = 0; a < D; a++)
    {
      m_iStarts[a] = iStarts[a];
      dimIndizes[a] = 0;
    }

    char* pointersPtr = m_pBase + m_dimProductArray[D - 1] * sizeof(T);
    m_pData = initPtr(0, dimIndizes, pointersPtr);
  };

  void setZeroBases()
  {
    // Set all start positions
    // to 0
    int iStarts[D];
    for(int a = 0; a < D; a++)
      iStarts[a] = 0;

    setBases(iStarts);
  };

  bool setValue(const int iPos[D], const T value)
  {
    // Check limits
    for(int a = 0; a < D; a++)
      if (iPos[a] < m_iStarts[a] || iPos[a] >= m_iStarts[a] + m_iLengths[a])
        return false;

    // Calculate the position
    // in the data memory 
    size_t posPtr = iPos[D - 1] - m_iStarts[D - 1];
    for(int a = 0; a < D - 1; a++)
    {
      posPtr += (iPos[a] - m_iStarts[a]) * m_PtrPartsProduct[a + 1];
    }
    // Set the value
    reinterpret_cast<T*>(m_pBase)[posPtr] = value;

    return true;
  };
  T value(const int iPos[D]) const
  {
    // Check limits
    for(int a = 0; a < D; a++)
      if (iPos[a] < m_iStarts[a] || iPos[a] >= m_iStarts[a] + m_iLengths[a])
        return 0;

    // Calculate the position
    // in the data memory 
    size_t posPtr = iPos[D - 1] - m_iStarts[D - 1];
    for(int a = 0; a < D - 1; a++)
    {
      posPtr += (iPos[a] - m_iStarts[a]) * m_PtrPartsProduct[a + 1];
    }
    // Get the value
    return reinterpret_cast<T*>(m_pBase)[posPtr];
  };

protected:
  // The following methods are only for inheritance, 
  // so hopefuly the developer give the count of 
  // arguments which corrospond with the count of 
  // dimensions D
  CMultiDimArray(int iFirstStart, int iFirstLength, ...)
    :m_size(0),
     m_pData(NULL),
     m_pBase(NULL)
  {
    va_list ap;
    
    m_iStarts[0] = iFirstStart;
    m_iLengths[0] = iFirstLength;
    va_start(ap, iFirstLength);
    for (int a=1; a < D; a++)
    {
      m_iStarts[a] = va_arg(ap, int);
      m_iLengths[a] = va_arg(ap, int);
    }
    va_end(ap);

    create();
  };
  
  void setBases(int iFirstStart, ...)
  {
    va_list ap;
    
    int iStarts[D];
    iStarts[0] = iFirstStart;
    va_start(ap, iFirstStart);
    for (int a=1; a < D; a++)
    {
      iStarts[a] = va_arg(ap, int);
    }
    va_end(ap);

    setBases(iStarts);
  };
  
  bool setValue(const T value, int iFirstPos, ...)
  {
    va_list ap;
    
    int iPos[D];
    iPos[0] = iFirstPos;
    va_start(ap, iFirstPos);
    for (int a=1; a < D; a++)
    {
      iPos[a] = va_arg(ap, int);
    }
    va_end(ap);

    return setValue(iPos, value);
  };
  
  T value(int iFirstPos, ...) const
  {
    va_list ap;
    
    int iPos[D];
    iPos[0] = iFirstPos;
    va_start(ap, iFirstPos);
    for (int a=1; a < D; a++)
    {
      iPos[a] = va_arg(ap, int);
    }
    va_end(ap);

    return value(iPos);
  };
  
  // Now, this are methods for creation
  // and destroying the array
  void create()
  {
    // Some checks
    for(int a = 0; a < D; a++)
      if(m_iLengths[a] < 1)
        throw std::bad_alloc();

    // Precalculate the size
    m_size = m_iLengths[0];
    for(int a = 1; a < D; a++)
      m_size *= m_iLengths[a];

    if(!allocate(NULL))
      throw std::bad_alloc();
  };
  
  void destroy()
  {
    if (m_pBase)
    {
      free(m_pBase);
      m_pBase = NULL;
    }
    m_pData = NULL;
    m_size = 0;
    for (int i=0; i < D; i++)
    {
      m_iStarts[i] = 0;
      m_iLengths[i] = 0;
    }
  };
  
  // Some member vars
  int m_iStarts[D];
  int m_iLengths[D];
  // The size of the array
  size_t m_size;
  // The pointer to the 
  // data
  char* m_pData;
  // The pointer to 
  // the base of data
  // for freeing the 
  // memory
  char* m_pBase;
private:
  // Helper functions for creation
  // of the array
  size_t offset(int iLevel) const;
  size_t doffset(int iLevel, size_t* dimIndizes, size_t dimProduct) const;
  char* initPtr(int iLevel, size_t* dimIndizes, char* pointersPtr);
  bool allocate(char* init_ptr);
  // Some private member vars
  // Helper array for fast calculation 
  // of pointers
  size_t m_PtrPartsProduct[D];
  // product of dimensions from 0 to index, 
  // m_iLengths[0]*m_iLengths[1]...m_iLengths[index]
  size_t m_dimProductArray[D];
};

template<class T>
class C1DArray: public CMultiDimArray<T, 1>
{
public:
  C1DArray(int iStart1, int iEnd1)
    :CMultiDimArray<T, 1>(iStart1, iEnd1 - iStart1 + 1)
  {};
  C1DArray(int iEnd1)
    :CMultiDimArray<T, 1>(0, iEnd1 + 1)
  {};

  T& operator[](int i) const {return reinterpret_cast<T*>(CMultiDimArray<T, 1>::m_pData)[i];};

  T* rawArray() const {return reinterpret_cast<T*>(CMultiDimArray<T, 1>::m_pData);};

  int firstStart() const {return CMultiDimArray<T, 1>::m_iStarts[0];};
  int firstLength() const {return CMultiDimArray<T, 1>::m_iLengths[0];};

  void setBases(int iStart1)
  {
    CMultiDimArray<T, 1>::setBases(iStart1);
  };

  bool setValue(int iPos1, const T value) 
  {
    return CMultiDimArray<T, 1>::setValue(value, iPos1);
  };
  T value(int iPos1) const
  {
    return CMultiDimArray<T, 1>::value(iPos1);
  };
  void dump(bool beautify = true) const
  {
    cout << dumpStr(beautify);
  };
  void dumpFile(const string& filename, bool beautify = false) const
  {
    ofstream ofile (filename.c_str());  
    ofile << dumpStr(beautify);;
  };
  string dumpStr(bool beautify = true) const
  {
    stringstream dumpStream; 
    if (beautify)
    {
      dumpStream << setw(4) << setfill(' ') << ' ' << '|' << setw(13) << setfill(' ') << 1 << endl;
      dumpStream << setw(4) << setfill('-') << '-' << '|' << setw(13) << setfill('-') << '-' << endl;
    }
    for (int i = CMultiDimArray<T, 1>::m_iStarts[0]; i < CMultiDimArray<T, 1>::m_iStarts[0] + CMultiDimArray<T, 1>::m_iLengths[0]; ++i)
    {
      if (beautify)
        dumpStream << setw(4) << setfill(' ') << i << '|';
      dumpStream << scientific << setw(13) << setfill(' ') << CMultiDimArray<T, 1>::value(i) << endl;
    }
    return dumpStream.str();
  };
};

template<class T>
class C2DArray: public CMultiDimArray<T, 2>
{
public:
  C2DArray(int iStart1, int iEnd1, 
           int iStart2, int iEnd2)
    :CMultiDimArray<T, 2>(iStart1, iEnd1 - iStart1 + 1,
                          iStart2, iEnd2 - iStart2 + 1)
  {};
  C2DArray(int iEnd1, 
           int iEnd2)
    :CMultiDimArray<T, 2>(0, iEnd1 + 1,
                          0, iEnd2 + 1)
  {};

  T*& operator[](int i) const {return reinterpret_cast<T**>(CMultiDimArray<T, 2>::m_pData)[i];};

  T** rawArray() const {return reinterpret_cast<T**>(CMultiDimArray<T, 2>::m_pData);};

  int firstStart() const {return CMultiDimArray<T, 2>::m_iStarts[0];};
  int secondStart() const {return CMultiDimArray<T, 2>::m_iStarts[1];};

  int firstLength() const {return CMultiDimArray<T, 2>::m_iLengths[0];};
  int secondLength() const {return CMultiDimArray<T, 2>::m_iLengths[1];};

  void setBases(int iStart1, int iStart2)
  {
    CMultiDimArray<T, 2>::setBases(iStart1, iStart2);
  };

  bool setValue(int iPos1, int iPos2, const T value) 
  {
    return CMultiDimArray<T, 2>::setValue(value, iPos1, iPos2);
  };
  T value(int iPos1, int iPos2) const
  {
    return CMultiDimArray<T, 2>::value(iPos1, iPos2);
  };
  void dump(bool beautify = true) const
  {
    cout << dumpStr(beautify);
  };
  void dumpFile(const string& filename, bool beautify = false) const
  {
    ofstream ofile (filename.c_str());  
    ofile << dumpStr(beautify);;
  };
  string dumpStr(bool beautify = true) const
  {
    stringstream dumpStream; 
    if (beautify)
    {
      dumpStream << setw(4) << setfill(' ') << ' ';
      for (int i = CMultiDimArray<T, 2>::m_iStarts[1]; i < CMultiDimArray<T, 2>::m_iStarts[1] + CMultiDimArray<T, 2>::m_iLengths[1]; ++i)
        dumpStream << "|" << setw(13) << setfill(' ') << i;
      dumpStream << endl;
      dumpStream << setw(4) << setfill('-') << '-';  
      for (int i = CMultiDimArray<T, 2>::m_iStarts[1]; i < CMultiDimArray<T, 2>::m_iStarts[1] + CMultiDimArray<T, 2>::m_iLengths[1]; ++i)
        dumpStream << "|" << setw(13) << setfill('-') << '-';
      dumpStream << endl;
    }
    for (int a = CMultiDimArray<T, 2>::m_iStarts[0]; a < CMultiDimArray<T, 2>::m_iStarts[0] + CMultiDimArray<T, 2>::m_iLengths[0]; ++a)
    {
      if (beautify)
        dumpStream << setw(4) << setfill(' ') << a;
      for (int i = CMultiDimArray<T, 2>::m_iStarts[1]; i < CMultiDimArray<T, 2>::m_iStarts[1] + CMultiDimArray<T, 2>::m_iLengths[1]; ++i)
      {
        if (beautify)
          dumpStream << "|";
        dumpStream << scientific << setw(13) << setfill(' ') << CMultiDimArray<T, 2>::value(a, i);
      }
      dumpStream << endl;
    }
    return dumpStream.str();
  };
};

template<class T>
class C3DArray: public CMultiDimArray<T, 3>
{
public:
  C3DArray(int iStart1, int iEnd1, 
           int iStart2, int iEnd2, 
           int iStart3, int iEnd3)
    :CMultiDimArray<T, 3>(iStart1, iEnd1 - iStart1 + 1, 
                          iStart2, iEnd2 - iStart2 + 1, 
                          iStart3, iEnd3 - iStart3 + 1)
  {};
  C3DArray(int iEnd1, 
           int iEnd2, 
           int iEnd3)
    :CMultiDimArray<T, 3>(0, iEnd1 + 1, 
                          0, iEnd2 + 1, 
                          0, iEnd3 + 1)
  {};

  T** operator[](int i) const {return reinterpret_cast<T***>(CMultiDimArray<T, 3>::m_pData)[i];};

  T*** rawArray() const {return reinterpret_cast<T***>(CMultiDimArray<T, 3>::m_pData);};

  int firstStart() const {return CMultiDimArray<T, 3>::m_iStarts[0];};
  int secondStart() const {return CMultiDimArray<T, 3>::m_iStarts[1];};
  int thirdStart() const {return CMultiDimArray<T, 3>::m_iStarts[2];};

  int firstLength() const {return CMultiDimArray<T, 3>::m_iLengths[0];};
  int secondLength() const {return CMultiDimArray<T, 3>::m_iLengths[1];};
  int thirdLength() const {return CMultiDimArray<T, 3>::m_iLengths[2];};

  void setBases(int iStart1, int iStart2, int iStart3)
  {
    CMultiDimArray<T, 3>::setBases(iStart1, iStart2, iStart3);
  };

  bool setValue(int iPos1, int iPos2, int iPos3, const T value) 
  {
    return CMultiDimArray<T, 3>::setValue(value, iPos1, iPos2, iPos3);
  };
  T value(int iPos1, int iPos2, int iPos3) const
  {
    return CMultiDimArray<T, 3>::value(iPos1, iPos2, iPos3);
  };
};

template<class T>
class C4DArray: public CMultiDimArray<T, 4>
{
public:
  C4DArray(int iStart1, int iEnd1, 
           int iStart2, int iEnd2, 
           int iStart3, int iEnd3,
           int iStart4, int iEnd4)
    :CMultiDimArray<T,4>(iStart1, iEnd1 - iStart1 + 1, 
                         iStart2, iEnd2 - iStart2 + 1, 
                         iStart3, iEnd3 - iStart3 + 1,
                         iStart4, iEnd4 - iStart4 + 1)
  {};
  C4DArray(int iEnd1, 
           int iEnd2, 
           int iEnd3,
           int iEnd4)
    :CMultiDimArray<T,4>(0, iEnd1 + 1, 
                         0, iEnd2 + 1, 
                         0, iEnd3 + 1,
                         0, iEnd4 + 1)
  {};

  T*** operator[](int i) const {return reinterpret_cast<T****>(CMultiDimArray<T, 4>::m_pData)[i];};

  T**** rawArray() const {return reinterpret_cast<T****>(CMultiDimArray<T, 4>::m_pData);};

  int firstStart() const {return CMultiDimArray<T, 4>::m_iStarts[0];};
  int secondStart() const {return CMultiDimArray<T, 4>::m_iStarts[1];};
  int thirdStart() const {return CMultiDimArray<T, 4>::m_iStarts[2];};
  int fourthStart() const {return CMultiDimArray<T, 4>::m_iStarts[3];};

  int firstLength() const {return CMultiDimArray<T, 4>::m_iLengths[0];};
  int secondLength() const {return CMultiDimArray<T, 4>::m_iLengths[1];};
  int thirdLength() const {return CMultiDimArray<T, 4>::m_iLengths[2];};
  int fourthLength() const {return CMultiDimArray<T, 4>::m_iLengths[3];};

  void setBases(int iStart1, int iStart2, int iStart3, int iStart4)
  {
    CMultiDimArray<T, 4>::setBases(iStart1, iStart2, iStart3, iStart4);
  };

  bool setValue(int iPos1, int iPos2, int iPos3, int iPos4, const T value) 
  {
    return CMultiDimArray<T, 4>::setValue(value, iPos1, iPos2, iPos3, iPos4);
  };
  T value(int iPos1, int iPos2, int iPos3, int iPos4) const
  {
    return CMultiDimArray<T, 4>::value(iPos1, iPos2, iPos3, iPos4);
  };
};

//
// Implementation of CMultiDynArray
//
template<class T, int D>
size_t CMultiDimArray<T, D>::offset(int iLevel) const
{
  if (iLevel == 0)
    return 0;
  else
    // Calculate the ptr offset for the given level
    return m_dimProductArray[iLevel - 1] + 
           offset(iLevel - 1);
};

template<class T, int D>
size_t CMultiDimArray<T, D>::doffset(int iLevel, size_t* dimIndizes, size_t dimProduct) const
{
  if (iLevel == 0)
    return 0;
  else
  {
    size_t r=doffset(iLevel - 1, dimIndizes, dimProduct) +
           (dimProduct / m_dimProductArray[iLevel - 1]) * dimIndizes[iLevel - 1];
    return r;
  }
};

template<class T, int D>
char* CMultiDimArray<T, D>::initPtr(int iLevel, size_t* dimIndizes, char* pointersPtr)
{

char** ptrs;
  // if level is not final level get pointer to array of pointers
  // for that level and recursively call ptr_init() to return pointers to
  // fill the array of pointers.  if level is final level calculate
  // position of data and return pointer to level-1.
  if (iLevel < (D - 1))
  {
    // ptrs points to the start of a subarray of pointers to the
    // next dimension level of pointers or data
    ptrs = reinterpret_cast<char**>(pointersPtr + offset(iLevel) * sizeof(char*) +
                                    ((iLevel == 0)?0:doffset(iLevel, dimIndizes,  m_dimProductArray[iLevel]) * sizeof(char*)));

    // fill the array of pointers that ptrs points to
    for (int i = 0; i < m_iLengths[iLevel]; i++)
    {
      dimIndizes[iLevel] = i;

      // the recursive call to initPtr() returns a pointer to the
      // next level and is then adjusted by m_iStarts[] for a
      // non-zero based subscript
      ptrs[i] = initPtr(iLevel + 1, dimIndizes, pointersPtr) - 
                m_iStarts[iLevel + 1] * ((iLevel + 1 == D - 1)?sizeof(T):sizeof(char*));
    }

    // adjust zeroth level pointer that is passed back to root
    // of recursive tree call
    if (iLevel == 0)
      ptrs -= m_iStarts[0];
  }
  else
  {
    // calculate pointers to data subarrays.  if one dimensional
    // array and non-zero subscripting adjust returned pointer
    ptrs = reinterpret_cast<char**>(m_pBase + 
                                    doffset(iLevel, dimIndizes, m_dimProductArray[iLevel]) * sizeof(T) - 
                                    ((iLevel == 0)?(m_iStarts[0] * sizeof(T)):0));
  }

  return reinterpret_cast<char*>(ptrs);
};

template<class T, int D>
bool CMultiDimArray<T, D>::allocate(char *init_ptr)
{
  // array of current dimension indices
  size_t dimIndizes[D];

  // Set all dim_ind to 0
  // Calculate the dp array so
  // every following index will
  // include the previous * the own
  // lenght of the index
  dimIndizes[0] = 0;
  m_dimProductArray[0] = m_iLengths[0];
  for (int i = 1; i < D; i++)
  {
    dimIndizes[i] = 0;
    m_dimProductArray[i] = m_dimProductArray[i - 1] * m_iLengths[i];
  }
  // Calculate the parts of the 
  // single array pointer for
  // fast linear pointer address calculation
  m_PtrPartsProduct[D - 1] = m_iLengths[D - 1];
  for(int a = D - 2; a >= 0; a--)
  {
    m_PtrPartsProduct[a] = m_PtrPartsProduct[a + 1] * m_iLengths[a];
  }
  // Allocate memory for 
  // * the data
  // * the pointer
  // * the alignement of pointer boundery
  m_pBase = static_cast<char*>(malloc(m_dimProductArray[D - 1] * sizeof(T) +
                                      offset(D - 1) * sizeof(char*) + 
                                      sizeof(char*)));
  // If base is 0 the memory
  // allocation failed, so stop
  // at this point
  if(m_pBase == NULL)
    return false;

  // Calculate start address of pointers
  // this will rekursiv changed in ptr_init
  char* pointersPtr = m_pBase + m_dimProductArray[D - 1] * sizeof(T);
  // Align start address to pointer boundery
  for (unsigned int i = 0 ; i < sizeof(char*); i++, ++pointersPtr)
  {
    if (reinterpret_cast<size_t>(pointersPtr) % sizeof(char*) == 0)
      break;
  }

  /* if init_ptr is NULL skip initialization */
  if (init_ptr != NULL)
  {
    /* set p_data to point to the start of the data area */
    char* p_data = m_pBase;

    /* initialize the array */
    for (size_t i = 0; i < m_dimProductArray[D - 1]; i++)
    {
      char* p = init_ptr;
      for (size_t j = 0; j < sizeof(T); j++)
      {
        *p_data++ = *p++;
      }
    }
  }

  // Calculate and set the pointers rekursiv
  m_pData = initPtr(0, dimIndizes, pointersPtr);

  // All went fine, return success
  return true;
};

#endif // CMULTIDIMARRAY_H
