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
# medio_install_tool_link(<target>)
#
# The command line tools are installed with their version in the name (e.g.
# bin/ecat2nifti-4.0.0), so that several versions can be installed next to each other.
# This function also installs a symbolic link without the version (bin/ecat2nifti ->
# ecat2nifti-4.0.0) that points to the version installed last. The link is relative,
# so the installation can be moved. Without symbolic links (Windows) or with
# MEDIO_INSTALL_TOOL_LINKS=OFF, nothing is done.

option(MEDIO_INSTALL_TOOL_LINKS
       "Install links without version (bin/<tool>) to the command line tools (bin/<tool>-<version>)" ON)

function(medio_install_tool_link target)
  if(WIN32 OR NOT MEDIO_INSTALL_TOOL_LINKS)
    return()
  endif()

  get_target_property(file_name ${target} OUTPUT_NAME)
  if(NOT file_name)
    set(file_name ${target})
  endif()
  # the name without the version suffix (-<major>.<minor>...)
  string(REGEX REPLACE "-[0-9]+(\\.[0-9A-Za-z]+)*$" "" link_name "${file_name}")
  if(link_name STREQUAL file_name)
    return()
  endif()

  install(CODE "
    set(_medio_bin \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin\")
    message(STATUS \"Installing: \${_medio_bin}/${link_name} -> ${file_name}\")
    file(CREATE_LINK \"${file_name}\" \"\${_medio_bin}/${link_name}\" SYMBOLIC)
  ")
endfunction()
