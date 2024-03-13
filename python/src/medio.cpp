#include "pmedio.h"

#include <CMedIO>
#include <CMedIOData>
#include <CMedIODataFactory>
#include <CMedIOHeader>

#include <QIODevice>

namespace py = pybind11;

// pybind init function for CMedIO classes
void init_medio(py::module_ &m)
{
  // CMedIO
  py::class_<CMedIO>(m, "MedIO")
    .def_static("majorVersion", &CMedIO::majorVersion)
    .def_static("minorVersion", &CMedIO::minorVersion)
    .def_static("version", &CMedIO::version)
    .def_static("buildDate", &CMedIO::buildDate)
    .def_static("copyright", &CMedIO::copyright)
    .def_static("name", &CMedIO::name)
    .def_static("qtMajorVersion", &CMedIO::qtMajorVersion)
    .def_static("qtMinorVersion", &CMedIO::qtMinorVersion)
    .def_static("qtBuildVersion", &CMedIO::qtBuildVersion)
    .def_static("qtVersion", &CMedIO::qtVersion);

  // CMedIOData
/*
  py::class_<CMedIOData> medioData(m, "MedIOData");
  medioData.def("open", &CMedIOData::open, py::arg("mode"))
           .def("close", &CMedIOData::close)
           .def("dataFormat", &CMedIOData::dataFormat)
           .def("userRtti", &CMedIOData::userRtti)
           .def("errorStatus", &CMedIOData::errorStatus);

  py::enum_<CMedIOData::Format>(medioData, "Format")
    .value("Unknown", CMedIOData::Unknown)
    .value("ConcordeMicropet", CMedIOData::ConcordeMicropet)
    .value("ECAT", CMedIOData::ECAT)
    .value("Philips", CMedIOData::Philips)
    .value("PhilipsBin", CMedIOData::PhilipsBin)
    .export_values();

  py::enum_<CMedIOData::errStatus>(medioData, "errStatus")
    .value("MERR_Ok", CMedIOData::MERR_Ok)
    .value("MERR_Unspecified", CMedIOData::MERR_Unspecified)
    .export_values();

  // CMedIODataFactory
  py::class_<CMedIODataFactory>(m, "MedIODataFactory")
    .def_static("identify", &CMedIODataFactory::identify, py::arg("fileName"))
    .def_static("create", &CMedIODataFactory::create, py::arg("fileName"));

  // CMedIOHeader
  py::class_<CMedIOHeader> medioHeader(m, "MedIOHeader");
  medioHeader.def("clear", &CMedIOHeader::clear)
             .def("load", &CMedIOHeader::load)
             .def("save", &CMedIOHeader::save)
             .def("headerFormat", &CMedIOHeader::headerFormat)
             .def("convertFrom", static_cast<bool (CMedIOHeader::*)(CMedIOData*)>(&CMedIOHeader::convertFrom),
                 py::arg("data"))
             .def("convertFrom", static_cast<bool (CMedIOHeader::*)(const CMedIOHeader*, const CMedIOHeader*)>(&CMedIOHeader::convertFrom), py::arg("header1"), py::arg("header2") = nullptr)
             .def("userRtti", &CMedIOHeader::userRtti)
             .def("clone", &CMedIOHeader::clone)
             .def_property("medIOData", &CMedIOHeader::medIOData, &CMedIOHeader::setMedIOData);

  py::enum_<CMedIOHeader::Format>(medioHeader, "Format")
    .value("Unknown", CMedIOHeader::Unknown)
    .value("ConcordeMicroPetMainHeader", CMedIOHeader::ConcordeMicroPetMainHeader)
    .value("ConcordeMicroPetFrameHeader", CMedIOHeader::ConcordeMicroPetFrameHeader)
    .value("ECATMainHeader", CMedIOHeader::ECATMainHeader)
    .value("ECATSubHeader", CMedIOHeader::ECATSubHeader)
    .value("PhilipsMainHeader", CMedIOHeader::PhilipsMainHeader)
    .value("PhilipsSubHeader", CMedIOHeader::PhilipsSubHeader)
    .value("PhilipsListviewHeader", CMedIOHeader::PhilipsListviewHeader)
    .export_values();
*/
}
