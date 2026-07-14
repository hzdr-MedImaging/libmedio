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

#include "pmedio.h"

#include <CMedIOHeader>
#include <CECATFile>
#include <CECATDirectory>
#include <CECATMainHeader>
#include <CECATSubHeader>
#include <CECAT7MainHeader>
#include <CECAT7SubHeaderAttenCorr>
#include <CECAT7SubHeaderImage>
#include <CECAT7SubHeaderNorm>
#include <CECAT7SubHeaderNorm3D>
#include <CECAT7SubHeaderPolarMap>
#include <CECAT7SubHeaderScan>
#include <CECAT7SubHeaderScan3D>

#include <iostream>

namespace py = pybind11;

// pybind init function for CECAT classes
void init_ecat(py::module_ &m)
{
  // CECATMainHeader
  py::class_<CECATMainHeader, CMedIOHeader> ecatMainHeader(m, "ECATMainHeader");
  ecatMainHeader.def("rawDataSize", &CECATMainHeader::rawDataSize)
                .def_property("num_Planes", &CECATMainHeader::num_Planes, &CECATMainHeader::setNum_Planes)
                .def_property("num_Frames", &CECATMainHeader::num_Frames, &CECATMainHeader::setNum_Frames)
                .def_property("num_Gates", &CECATMainHeader::num_Gates, &CECATMainHeader::setNum_Gates)
                .def_property("num_Bed_Pos", &CECATMainHeader::num_Bed_Pos, &CECATMainHeader::setNum_Bed_Pos)
                .def("mainHeaderType", &CECATMainHeader::mainHeaderType)
                .def("clone", &CECATMainHeader::clone)
                .def("convertFrom", &CECATMainHeader::convertFrom,
                    py::arg("mainHeader"),
                    py::arg("subHeader") = nullptr);

  py::enum_<CECATMainHeader::HeaderType>(ecatMainHeader, "HeaderType")
    .value("UnknownHeaderType", CECATMainHeader::UnknownHeaderType)
    .value("ECAT6MainHeader", CECATMainHeader::ECAT6MainHeader)
    .value("ECAT7MainHeader", CECATMainHeader::ECAT7MainHeader)
    .export_values();

  py::enum_<CECATMainHeader::Type>(ecatMainHeader, "Type")
    .value("Unknown", CECATMainHeader::Unknown)
    .value("ECAT7_Sinogram", CECATMainHeader::ECAT7_Sinogram)
    .value("ECAT7_Image16", CECATMainHeader::ECAT7_Image16)
    .value("ECAT7_AttenCorr", CECATMainHeader::ECAT7_AttenCorr)
    .value("ECAT7_Normalization", CECATMainHeader::ECAT7_Normalization)
    .value("ECAT7_PolarMap", CECATMainHeader::ECAT7_PolarMap)
    .value("ECAT7_Volume8", CECATMainHeader::ECAT7_Volume8)
    .value("ECAT7_Volume16", CECATMainHeader::ECAT7_Volume16)
    .value("ECAT7_Projection8", CECATMainHeader::ECAT7_Projection8)
    .value("ECAT7_Projection16", CECATMainHeader::ECAT7_Projection16)
    .value("ECAT7_Image8", CECATMainHeader::ECAT7_Image8)
    .value("ECAT7_Sinogram3D_16", CECATMainHeader::ECAT7_Sinogram3D_16)
    .value("ECAT7_Sinogram3D_8", CECATMainHeader::ECAT7_Sinogram3D_8)
    .value("ECAT7_Normalization_3D", CECATMainHeader::ECAT7_Normalization_3D)
    .value("ECAT7_Sinogram3D_Float", CECATMainHeader::ECAT7_Sinogram3D_Float)
    .export_values();

  // CECATFile
  py::class_<CECATFile, CMedIOData> ecatFile(m, "ECATFile");
  ecatFile.def(py::init<const QString&, CECATMainHeader::Type>(),
              py::arg("fileName"),
              py::arg("fileType") = CECATMainHeader::Unknown)
          .def(py::init<CECATMainHeader::Type>(),
              py::arg("fileType") = CECATMainHeader::Unknown)
          .def_static("isOfType", &CECATFile::isOfType,
              py::arg("fileName"))
          .def_static("createFromFile", &CECATFile::createFromFile,
              py::arg("fileName"))
          .def("format", &CECATFile::format)
          .def_property("fileType", &CECATFile::fileType, &CECATFile::setFileType)
          .def("subHeaderType", static_cast<CECATSubHeader::Type (CECATFile::*)()>(&CECATFile::subHeaderType))
          .def("subHeaderType", static_cast<CECATSubHeader::Type (CECATFile::*)(CECATMainHeader::Type)>(&CECATFile::subHeaderType),
              py::arg("fileType"))
          .def("maxFrame", &CECATFile::maxFrame)
          .def("minFrame", &CECATFile::minFrame)
          .def("numFrames", &CECATFile::numFrames)
          .def("maxPlane", &CECATFile::maxPlane)
          .def("minPlane", &CECATFile::minPlane)
          .def("numPlanes", &CECATFile::numPlanes)
          .def("maxGate", &CECATFile::maxGate)
          .def("minGate", &CECATFile::minGate)
          .def("numGates", &CECATFile::numGates)
          .def("maxBedPos", &CECATFile::maxBedPos)
          .def("minBedPos", &CECATFile::minBedPos)
          .def("numBedPos", &CECATFile::numBedPos)
          .def("maxData", &CECATFile::maxData)
          .def("minData", &CECATFile::minData)
          .def("numData", &CECATFile::numData)
          .def("readMainHeader", [](CECATFile& self)
          {
            CECATMainHeader* ecatMainHeader = nullptr;
            self.readMainHeader(ecatMainHeader);
            return ecatMainHeader;
          })
          .def("readSubHeader", [](CECATFile& self, short frame=-1, short plane=-1, short gate=-1, short bed=-1, short data=-1)
          {
            CECATSubHeader* ecatSubHeader = nullptr;
            self.readSubHeader(ecatSubHeader, frame, plane, gate, bed, data);
            return ecatSubHeader;
          },  py::arg("frame") = -1,
              py::arg("plane") = -1,
              py::arg("gate") = -1,
              py::arg("bed") = -1,
              py::arg("data") = -1)
         .def("readMatrix", [](CECATFile& self,
                               const std::vector<int>& volumes={},
                               const std::vector<int>& rows={},
                               const std::vector<int>& cols={},
                               const std::vector<int>& planes={})
          {
            MedIOImage medIOImage(&self);
            return medIOImage.readImage(volumes, rows, cols, planes);
          },  py::arg("volumes") = std::vector<int>(),
              py::arg("rows") = std::vector<int>(),
              py::arg("cols") = std::vector<int>(),
              py::arg("planes") = std::vector<int>())
          .def("writeMainHeader", &CECATFile::writeMainHeader,
              py::arg("mainHeader"))
          .def("writeSubHeader", &CECATFile::writeSubHeader,
              py::arg("subHeader"),
              py::arg("frame") = -1,
              py::arg("plane") = -1,
              py::arg("gate") = -1,
              py::arg("bed") = -1,
              py::arg("data") = -1)
          .def("writeMatrix", [](CECATFile& self,
                                 const MedIOImage& img,
                                 short frame=-1,
                                 short plane=-1,
                                 short gate=-1,
                                 short bed=-1,
                                 short data=-1)
          {
            MedIOImage medIOImage(&self, img);
            return medIOImage.ndim();
          },  py::arg("img"),
              py::arg("frame") = -1,
              py::arg("plane") = -1,
              py::arg("gate") = -1,
              py::arg("bed") = -1,
              py::arg("data") = -1)
          .def("writeMatrix", [](CECATFile& self,
                                 const py::array_t<float, py::array::f_style | py::array::forcecast>& img,
                                 short frame=-1,
                                 short plane=-1,
                                 short gate=-1,
                                 short bed=-1,
                                 short data=-1)
          {
            py::buffer_info buf = img.request();
            return buf.ndim;
          },  py::arg("img"),
              py::arg("frame") = -1,
              py::arg("plane") = -1,
              py::arg("gate") = -1,
              py::arg("bed") = -1,
              py::arg("data") = -1)
          //.def("writeMatrix", static_cast<bool (CECATFile::*)(const char*, unsigned int, short, short, short, short, short)>(&CECATFile::writeMatrix),
          //    py::arg("matrixData"),
          //    py::arg("size"),
          //    py::arg("frame") = -1,
          //    py::arg("plane") = -1,
          //    py::arg("gate") = -1,
          //    py::arg("bed") = -1,
          //    py::arg("data") = -1)
          //.def("writeMatrix", static_cast<bool (CECATFile::*)(const char*, unsigned int, const CECATSubHeader&, short, short, short, short, short)>(&CECATFile::writeMatrix),
          //    py::arg("matrixData"),
          //    py::arg("size"),
          //    py::arg("subHeader"),
          //    py::arg("frame") = -1,
          //    py::arg("plane") = -1,
          //    py::arg("gate") = -1,
          //    py::arg("bed") = -1,
          //    py::arg("data") = -1)
          //.def("writeMatrix", static_cast<bool (CECATFile::*)(const char*, unsigned int, const CECATSubHeader::Data_Type, short, short, short, short, short)>(&CECATFile::writeMatrix),
          //    py::arg("matrixData"),
          //    py::arg("size"),
          //    py::arg("type"),
          //    py::arg("frame") = -1,
          //    py::arg("plane") = -1,
          //    py::arg("gate") = -1,
          //    py::arg("bed") = -1,
          //    py::arg("data") = -1)
          .def("createEmptyMainHeader", &CECATFile::createEmptyMainHeader)
          .def("createEmptySubHeader", &CECATFile::createEmptySubHeader)
          .def("directory", &CECATFile::directory);

  py::enum_<CECATFile::ECATFormat>(ecatFile, "ECATFormat")
    .value("Undefined", CECATFile::Undefined)
    .value("ECAT7", CECATFile::ECAT7)
    .value("ECAT6", CECATFile::ECAT6)
    .export_values();

  // CECATSubHeader
  py::class_<CECATSubHeader, CMedIOHeader> ecatSubHeader(m, "ECATSubHeader");
  ecatSubHeader.def("rawDataSize", &CECATSubHeader::rawDataSize)
               .def_property("data_Type", &CECATSubHeader::data_Type, &CECATSubHeader::setData_Type)
               .def("subHeaderType", &CECATSubHeader::subHeaderType)
               .def("setDirectoryItem", &CECATSubHeader::setDirectoryItem,
                   py::arg("dItem"))
               .def("convertFrom", &CECATSubHeader::convertFrom,
                   py::arg("subHeader"),
                   py::arg("mainHeader") = nullptr);

  py::enum_<CECATSubHeader::Type>(ecatSubHeader, "Type")
    .value("Unknown", CECATSubHeader::Unknown)
    .value("ECAT7_AttenCorr", CECATSubHeader::ECAT7_AttenCorr)
    .value("ECAT7_Image", CECATSubHeader::ECAT7_Image)
    .value("ECAT7_Norm", CECATSubHeader::ECAT7_Norm)
    .value("ECAT7_Norm3D", CECATSubHeader::ECAT7_Norm3D)
    .value("ECAT7_PolarMap", CECATSubHeader::ECAT7_PolarMap)
    .value("ECAT7_Scan", CECATSubHeader::ECAT7_Scan)
    .value("ECAT7_Scan3D", CECATSubHeader::ECAT7_Scan3D)
    .export_values();

  py::enum_<CECATSubHeader::Data_Type>(ecatSubHeader, "Data_Type")
    .value("UnknownDataType", CECATSubHeader::UnknownDataType)
    .value("ByteData", CECATSubHeader::ByteData)
    .value("VAX_Ix2", CECATSubHeader::VAX_Ix2)
    .value("VAX_Ix4", CECATSubHeader::VAX_Ix4)
    .value("VAX_Rx4", CECATSubHeader::VAX_Rx4)
    .value("IEEEFloat", CECATSubHeader::IEEEFloat)
    .value("SunShort", CECATSubHeader::SunShort)
    .value("SunLong", CECATSubHeader::SunLong)
    .export_values();

  // CECATDirectory
  py::class_<CECATDirectory> ecatDirectory(m, "ECATDirectory");
  ecatDirectory.def(py::init<CECATFile*>(),
                   py::arg("eacatFile"))
               .def("load", &CECATDirectory::load)
               .def("save", &CECATDirectory::save)
               .def("isEmpty", &CECATDirectory::isEmpty)
               .def("count", &CECATDirectory::count)
               .def("__getitem__", &CECATDirectory::operator[], py::arg("num"))
               .def("item", &CECATDirectory::item,
                   py::arg("frame") = -1,
                   py::arg("plane") = -1,
                   py::arg("gate") = -1,
                   py::arg("bed") = -1,
                   py::arg("data") = -1)
               .def("maxFrame", &CECATDirectory::maxFrame)
               .def("minFrame", &CECATDirectory::minFrame)
               .def("numFrames", &CECATDirectory::numFrames)
               .def("maxPlane", &CECATDirectory::maxPlane)
               .def("minPlane", &CECATDirectory::minPlane)
               .def("numPlanes", &CECATDirectory::numPlanes)
               .def("maxGate", &CECATDirectory::maxGate)
               .def("minGate", &CECATDirectory::minGate)
               .def("numGates", &CECATDirectory::numGates)
               .def("maxBedPos", &CECATDirectory::maxBedPos)
               .def("minBedPos", &CECATDirectory::minBedPos)
               .def("numBedPos", &CECATDirectory::numBedPos)
               .def("maxData", &CECATDirectory::maxData)
               .def("minData", &CECATDirectory::minData)
               .def("numData", &CECATDirectory::numData)
               //.def("readSubHeader", &CECATDirectory::readSubHeader)
               //.def("readMatrix", static_cast<bool (CECATFile::*)(char*&, unsigned int&, short, short, short, short, short)>(&CECATDirectory::readMatrix))
               .def("writeSubHeader", &CECATDirectory::writeSubHeader,
                   py::arg("subHeader"),
                   py::arg("frame") = -1,
                   py::arg("plane") = -1,
                   py::arg("gate") = -1,
                   py::arg("bed") = -1,
                   py::arg("data") = -1)
               .def("writeMatrix", static_cast<bool (CECATDirectory::*)(const char*, unsigned int, short, short, short, short, short)>(&CECATDirectory::writeMatrix), 
                   py::arg("matrix"),
                   py::arg("size"),
                   py::arg("frame") = -1,
                   py::arg("plane") = -1,
                   py::arg("gate") = -1,
                   py::arg("bed") = -1,
                   py::arg("data") = -1)
               .def("writeMatrix", static_cast<bool (CECATDirectory::*)(const char*, unsigned int, const CECATSubHeader&, short, short, short, short, short)>(&CECATDirectory::writeMatrix),
                   py::arg("matrix"),
                   py::arg("size"),
                   py::arg("subHeader"),
                   py::arg("frame") = -1,
                   py::arg("plane") = -1,
                   py::arg("gate") = -1,
                   py::arg("bed") = -1,
                   py::arg("data") = -1)
               .def("writeMatrix", static_cast<bool (CECATDirectory::*)(const char*, unsigned int, const CECATSubHeader::Data_Type, short, short, short, short, short)>(&CECATDirectory::writeMatrix),
                   py::arg("matrix"),
                   py::arg("size"),
                   py::arg("type"),
                   py::arg("frame") = -1,
                   py::arg("plane") = -1,
                   py::arg("gate") = -1,
                   py::arg("bed") = -1,
                   py::arg("data") = -1);

  // CECATDirectoryItem
  py::class_<CECATDirectoryItem> ecatDirectoryItem(m, "ECATDirectoryItem");
  ecatDirectoryItem.def(py::init<CECATFile*, qint32>(),
                       py::arg("file"),
                       py::arg("matrixID") = 0)
                   .def_property("matrixID", &CECATDirectoryItem::matrixID, &CECATDirectoryItem::setMatrixID)
                   .def_property("dataBlock_Start", &CECATDirectoryItem::dataBlock_Start, &CECATDirectoryItem::setDataBlock_Start)
                   .def_property("dataBlock_End", &CECATDirectoryItem::dataBlock_End, &CECATDirectoryItem::setDataBlock_End)
                   .def_property("dataBlock_Status", &CECATDirectoryItem::dataBlock_Status, &CECATDirectoryItem::setDataBlock_Status)
                   .def_property("frame", &CECATDirectoryItem::frame, &CECATDirectoryItem::setFrame)
                   .def_property("plane", &CECATDirectoryItem::plane, &CECATDirectoryItem::setPlane)
                   .def_property("gate", &CECATDirectoryItem::gate, &CECATDirectoryItem::setGate)
                   .def_property("bed", &CECATDirectoryItem::bed, &CECATDirectoryItem::setBed)
                   .def_property("data", &CECATDirectoryItem::data, &CECATDirectoryItem::setData)
                   //.def("readSubHeader", &CECATDirectoryItem::readSubHeader)
                   //.def("readMatrix", static_cast<bool (CECATDirectoryItem::*)(char*&, unsigned int&)>(&CECATDirectoryItem::readMatrix))
                   //.def("readMatrix", static_cast<bool (CECATDirectoryItem::*)(char*&, unsigned int&, CECATSubHeader*&)>(&CECATDirectoryItem::readMatrix))
                   .def("writeSubHeader", &CECATDirectoryItem::writeSubHeader,
                       py::arg("subHeader"))
                   .def("writeMatrix", static_cast<bool (CECATDirectoryItem::*)(const char*, unsigned int)>(&CECATDirectoryItem::writeMatrix),
                       py::arg("data"),
                       py::arg("len"))
                   .def("writeMatrix", static_cast<bool (CECATDirectoryItem::*)(const char*, unsigned int, const CECATSubHeader&)>(&CECATDirectoryItem::writeMatrix),
                       py::arg("data"),
                       py::arg("len"),
                       py::arg("subHeader"))
                   .def("writeMatrix", static_cast<bool (CECATDirectoryItem::*)(const char*, unsigned int, const CECATSubHeader::Data_Type)>(&CECATDirectoryItem::writeMatrix),
                       py::arg("data"),
                       py::arg("len"),
                       py::arg("type"));
 
  py::enum_<CECATDirectoryItem::AccessStatus>(ecatDirectoryItem, "AccessStatus")
    .value("Deleted", CECATDirectoryItem::Deleted)
    .value("NotYetWritten", CECATDirectoryItem::NotYetWritten)
    .value("Finished", CECATDirectoryItem::Finished)
    .export_values();
}
