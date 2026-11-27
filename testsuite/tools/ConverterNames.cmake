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

# Converts the ECAT7 and Philips test datasets into NIfTI and back, calling the reverse
# direction by the second name of the program (nifti2ecat, nifti2philips: like
# ecat2nifti and philips2nifti a link to medioConvert, a copy on Windows), and checks the
# help of both names.
#
# cmake -DECAT2NIFTI=<program> -DPHILIPS2NIFTI=<program> -DDATA=<test data> -DWORK=<dir>
#       -P ConverterNames.cmake

file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")

# the second name: the same file name with the other direction
function(alias_of program from to result)
  get_filename_component(dir "${program}" DIRECTORY)
  get_filename_component(name "${program}" NAME)
  string(REPLACE "${from}" "${to}" name "${name}")
  if(NOT EXISTS "${dir}/${name}")
    message(FATAL_ERROR "${dir}/${name} does not exist")
  endif()
  set(${result} "${dir}/${name}" PARENT_SCOPE)
endfunction()

function(run expected_output)
  execute_process(COMMAND ${ARGN} RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
  message(STATUS "${ARGN}: ${output}${error}")
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "failed (${result}): ${ARGN}")
  endif()
  if(NOT output MATCHES "${expected_output}")
    message(FATAL_ERROR "the output of '${ARGN}' does not match '${expected_output}'")
  endif()
endfunction()

alias_of("${ECAT2NIFTI}" ecat2nifti nifti2ecat NIFTI2ECAT)
alias_of("${PHILIPS2NIFTI}" philips2nifti nifti2philips NIFTI2PHILIPS)

# the help of each name describes its own direction
run("<input\\.v>.*Called as nifti2ecat" "${ECAT2NIFTI}" --help)
run("\\[output\\.v\\].*Called as ecat2nifti" "${NIFTI2ECAT}" --help)
run("<input\\.img\\|input\\.syn>.*Called as nifti2philips" "${PHILIPS2NIFTI}" --help)
run("\\[output\\.img\\|output\\.syn\\].*Called as philips2nifti" "${NIFTI2PHILIPS}" --help)

# ECAT7 -> NIfTI -> ECAT7
run("ecat\\.nii\\.gz" "${ECAT2NIFTI}" "${DATA}/ecat/pet-dynamic.v" "${WORK}/ecat.nii.gz")
run("ecat\\.v" "${NIFTI2ECAT}" "${WORK}/ecat.nii.gz" "${WORK}/ecat.v")
file(READ "${WORK}/ecat.v" magic LIMIT 7 HEX)
if(NOT magic STREQUAL "4d415452495837")   # MATRIX7
  message(FATAL_ERROR "${WORK}/ecat.v is no ECAT7 file")
endif()
# the written file is an ECAT7 file that the other direction reads again
run("ecat2\\.nii\\.gz" "${ECAT2NIFTI}" "${WORK}/ecat.v" "${WORK}/ecat2.nii.gz")

# Philips -> NIfTI -> Philips
run("philips\\.nii\\.gz" "${PHILIPS2NIFTI}" "${DATA}/philips/pet-dynamic.img" "${WORK}/philips.nii.gz")
run("philips\\.img" "${NIFTI2PHILIPS}" "${WORK}/philips.nii.gz" "${WORK}/philips.img")
# the written file is a Philips file that the other direction reads again
run("philips2\\.nii\\.gz" "${PHILIPS2NIFTI}" "${WORK}/philips.img" "${WORK}/philips2.nii.gz")
