/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2026 hzdr.de and contributors
 
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

// nanobind type casters of the Qt types and enumerations used by pmedio and the
// names of formats, dimension types and data types in Python
#ifndef PMEDIO_CASTERS_H
#define PMEDIO_CASTERS_H

#include <nanobind/nanobind.h>

#include <CMedIOData>
#include <CMedIODataFactory>
#include <CECATSubHeader>

#include <QString>

#include <stdexcept>
#include <string>

// for the tdim dimension we use here
enum dimTypes {
  DM_UNKNOWN = 'u',
  DM_FRAMES = 'f',
  DM_GATES = 'g',
  DM_BEDS = 'b',
  DM_DATA = 'd'
};

enum dataTypes {
  DT_FLOAT = CECATSubHeader::IEEEFloat,  // 32bit float big endian
  DT_LONG = CECATSubHeader::SunLong,     // 32bit integer big endian
  DT_SHORT = CECATSubHeader::SunShort,   // 16bit short big endian
  DT_BYTE = CECATSubHeader::ByteData     // 8bit byte
};

// conversions between the names used in Python and the enumerations; the
// functions return false for unknown names
inline bool formatFromName(const QString& str, CMedIOData::Format& value)
{
  return CMedIODataFactory::formatFromName(str, value);
}

inline const char* formatName(CMedIOData::Format format)
{
  return CMedIODataFactory::formatName(format);
}

inline bool dimTypeFromName(const QString& str, enum dimTypes& value)
{
  if(str.startsWith('f', Qt::CaseInsensitive))
    value = DM_FRAMES;
  else if(str.startsWith('g', Qt::CaseInsensitive))
    value = DM_GATES;
  else if(str.startsWith('b', Qt::CaseInsensitive))
    value = DM_BEDS;
  else if(str.startsWith('d', Qt::CaseInsensitive))
    value = DM_DATA;
  else if(str.startsWith('u', Qt::CaseInsensitive))
    value = DM_UNKNOWN;
  else
    return false;

  return true;
}

inline const char* dimTypeName(enum dimTypes type)
{
  switch(type)
  {
    case DM_UNKNOWN: return "unknown";
    case DM_FRAMES:  return "frames";
    case DM_GATES:   return "gates";
    case DM_BEDS:    return "beds";
    case DM_DATA:    return "data";
  }
  return "";
}

inline bool dataTypeFromName(const QString& str, enum dataTypes& value)
{
  if(str.startsWith('f', Qt::CaseInsensitive))
    value = DT_FLOAT;
  else if(str.startsWith('s', Qt::CaseInsensitive))
    value = DT_SHORT;
  else if(str.startsWith('i', Qt::CaseInsensitive))
    value = DT_LONG;
  else if(str.startsWith('b', Qt::CaseInsensitive))
    value = DT_BYTE;
  else
    return false;

  return true;
}

inline const char* dataTypeName(enum dataTypes type)
{
  switch(type)
  {
    case DT_FLOAT: return "float";
    case DT_SHORT: return "short";
    case DT_LONG:  return "int";
    case DT_BYTE:  return "byte";
  }
  return "";
}

// conversions of the names given in Python that throw for unknown names
// (RuntimeError in Python)
inline CMedIOData::Format formatOfName(const QString& name)
{
  CMedIOData::Format value = CMedIOData::Unknown;
  if(formatFromName(name, value) == false)
    throw std::runtime_error("could not convert '" + name.toStdString() + "' to valid format type");
  return value;
}

inline enum dimTypes dimTypeOfName(const QString& name)
{
  enum dimTypes value = DM_UNKNOWN;
  if(dimTypeFromName(name, value) == false)
    throw std::runtime_error("could not convert '" + name.toStdString() + "' to valid dimension type");
  return value;
}

inline enum dataTypes dataTypeOfName(const QString& name)
{
  enum dataTypes value = DT_FLOAT;
  if(dataTypeFromName(name, value) == false)
    throw std::runtime_error("could not convert '" + name.toStdString() + "' to valid data type");
  return value;
}

// A bool argument that also takes objects with a truth value of their own
// (int, numpy.bool_, ...) and None (false), but no str or containers. This is
// what pybind11 accepted; the bool caster of nanobind only takes True and False.
struct Flag {
  bool value;
  operator bool() const { return value; }
};

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

// QString <> Python 'str'
template <> struct type_caster<QString> {
  NB_TYPE_CASTER(QString, const_name("str"))

  bool from_python(handle src, uint32_t, cleanup_list*) noexcept {
    if(!PyUnicode_Check(src.ptr()))
      return false;

    Py_ssize_t size = 0;
    const char* ptr = PyUnicode_AsUTF8AndSize(src.ptr(), &size);
    if(ptr == nullptr) {
      PyErr_Clear();
      return false;
    }

    value = QString::fromUtf8(ptr, static_cast<int>(size));
    return true;
  }

  static handle from_cpp(const QString& src, rv_policy, cleanup_list*) noexcept {
    const QByteArray utf8 = src.toUtf8();
    return PyUnicode_FromStringAndSize(utf8.constData(), utf8.size());
  }
};

// see Flag
template <> struct type_caster<Flag> {
  NB_TYPE_CASTER(Flag, const_name("bool"))

  bool from_python(handle src, uint32_t, cleanup_list*) noexcept {
    PyObject* object = src.ptr();
    if(object == Py_True || object == Py_False || object == Py_None) {
      value.value = (object == Py_True);
      return true;
    }

    // only types that define __bool__ (not the length of a str or list)
    typedef int (*inquiry_t)(PyObject*);
    inquiry_t truth = reinterpret_cast<inquiry_t>(PyType_GetSlot(Py_TYPE(object), Py_nb_bool));
    if(truth == nullptr)
      return false;

    const int result = truth(object);
    if(result != 0 && result != 1) {
      PyErr_Clear();
      return false;
    }

    value.value = (result == 1);
    return true;
  }

  static handle from_cpp(Flag src, rv_policy, cleanup_list*) noexcept {
    return handle(src.value ? Py_True : Py_False).inc_ref();
  }
};

// An enumeration that is passed and returned as Python 'str' (see the names
// above). from_python() must not throw and rejects unknown names; functions
// that report them with a RuntimeError take a str and use formatOfName(),
// dimTypeOfName() or dataTypeOfName().
template <typename Enum, bool (*FromName)(const QString&, Enum&), const char* (*ToName)(Enum)>
struct named_enum_caster {
  NB_TYPE_CASTER(Enum, const_name("str"))

  bool from_python(handle src, uint32_t, cleanup_list*) noexcept {
    if(!PyUnicode_Check(src.ptr()))
      return false;

    Py_ssize_t size = 0;
    const char* ptr = PyUnicode_AsUTF8AndSize(src.ptr(), &size);
    if(ptr == nullptr) {
      PyErr_Clear();
      return false;
    }

    return FromName(QString::fromUtf8(ptr, static_cast<int>(size)), value);
  }

  static handle from_cpp(Enum src, rv_policy, cleanup_list*) noexcept {
    return PyUnicode_FromString(ToName(src));
  }
};

template <> struct type_caster<CMedIOData::Format>
  : named_enum_caster<CMedIOData::Format, formatFromName, formatName> { };
template <> struct type_caster<enum dimTypes>
  : named_enum_caster<enum dimTypes, dimTypeFromName, dimTypeName> { };
template <> struct type_caster<enum dataTypes>
  : named_enum_caster<enum dataTypes, dataTypeFromName, dataTypeName> { };

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)

#endif
