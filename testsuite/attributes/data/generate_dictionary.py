#!/usr/bin/env python3
#
# libmedio - C++ I/O Library for loading/saving medical data formats
#            https://github.com/hzdr-MedImaging/libmedio
#
# Copyright (C) 2004-2026 hzdr.de and contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Generates dicom_dictionary.json: tag, VR and VM of the DICOM keywords used
# as attributes by libmedio (src/CMedIOHeaderDictionary.cpp), taken from the DICOM
# data dictionary of pydicom as independent source. testMedIOAttributes checks
# the dictionary of libmedio against this file.
#
# Usage: python3 generate_dictionary.py [output directory]

import json
import os
import sys

import pydicom
from pydicom.datadict import dictionary_VM, dictionary_VR, tag_for_keyword

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))

# the attributes of libmedio and the sequences holding some of them
KEYWORDS = [
    "PatientName", "PatientID", "PatientBirthDate", "PatientSex", "PatientAge",
    "PatientSize", "PatientWeight", "PatientPosition",
    "StudyInstanceUID", "SeriesInstanceUID", "FrameOfReferenceUID", "StudyID",
    "SeriesNumber", "AccessionNumber", "AcquisitionDateTime", "StudyDescription",
    "SeriesDescription", "InstitutionName", "ReferringPhysicianName",
    "RequestingPhysician", "OperatorsName",
    "Manufacturer", "ManufacturerModelName", "DeviceSerialNumber", "SoftwareVersions",
    "Radiopharmaceutical", "RadiopharmaceuticalRoute", "RadiopharmaceuticalStartDateTime",
    "RadionuclideTotalDose", "RadionuclideHalfLife", "RadionuclidePositronFraction",
    "RadionuclideCodeSequence", "RadiopharmaceuticalCodeSequence",
    "EnergyWindowLowerLimit", "EnergyWindowUpperLimit",
    "Units", "DecayCorrection", "CorrectedImage", "ReconstructionMethod",
    "AttenuationCorrectionMethod", "ScatterCorrectionMethod",
    "ActualFrameDuration", "DecayFactor", "DeadTimeFactor",
]

SEQUENCES = ["RadiopharmaceuticalInformationSequence", "EnergyWindowRangeSequence"]


def entry(keyword):
    tag = tag_for_keyword(keyword)
    return {"tag": "(%04X,%04X)" % (tag >> 16, tag & 0xFFFF),
            "vr": dictionary_VR(tag), "vm": dictionary_VM(tag)}


def main():
    result = {
        "generator": "pydicom %s" % pydicom.__version__,
        "keys": {k: entry(k) for k in KEYWORDS},
        "sequences": {k: entry(k) for k in SEQUENCES},
    }
    with open(os.path.join(OUT, "dicom_dictionary.json"), "w", encoding="utf-8") as f:
        json.dump(result, f, indent=2)
        f.write("\n")


if __name__ == "__main__":
    main()
