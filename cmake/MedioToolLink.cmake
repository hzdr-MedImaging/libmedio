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
#
# medio_tool_alias(<target> <alias>)
#
# A second name of a tool that selects another operation by the name it is called with
# (e.g. ecat2nifti for medioConvert): <alias>-<version> is a relative symbolic link to
# <target>-<version> (a copy on Windows) in the build tree and in the installation, and
# the installation also gets the link without the version (bin/nifti2ecat ->
# nifti2ecat-4.0.0), like medio_install_tool_link().

option(MEDIO_INSTALL_TOOL_LINKS
       "Install links without version (bin/<tool>) to the command line tools (bin/<tool>-<version>)" ON)

# installs the relative symbolic link bin/<link> -> <file> (replacing an existing one)
function(_medio_install_link link file)
  install(CODE "
    set(_medio_bin \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin\")
    message(STATUS \"Installing: \${_medio_bin}/${link} -> ${file}\")
    file(CREATE_LINK \"${file}\" \"\${_medio_bin}/${link}\" SYMBOLIC)
  ")
endfunction()

# the file name (OUTPUT_NAME) of a target and its version suffix (-4.0.0)
function(_medio_tool_name target file_var suffix_var)
  get_target_property(file_name ${target} OUTPUT_NAME)
  if(NOT file_name)
    set(file_name ${target})
  endif()
  string(REGEX MATCH "-[0-9]+(\\.[0-9A-Za-z]+)*$" suffix "${file_name}")
  set(${file_var} "${file_name}" PARENT_SCOPE)
  set(${suffix_var} "${suffix}" PARENT_SCOPE)
endfunction()

function(medio_install_tool_link target)
  if(WIN32 OR NOT MEDIO_INSTALL_TOOL_LINKS)
    return()
  endif()

  _medio_tool_name(${target} file_name suffix)
  if(NOT suffix)
    return()
  endif()
  string(LENGTH "${file_name}" length)
  string(LENGTH "${suffix}" suffix_length)
  math(EXPR length "${length} - ${suffix_length}")
  string(SUBSTRING "${file_name}" 0 ${length} link_name)
  _medio_install_link("${link_name}" "${file_name}")
endfunction()

function(medio_tool_alias target alias)
  _medio_tool_name(${target} file_name suffix)
  set(file "${file_name}${CMAKE_EXECUTABLE_SUFFIX}")
  set(alias_file "${alias}${suffix}${CMAKE_EXECUTABLE_SUFFIX}")

  if(WIN32)
    # no symbolic links: a copy, in the build tree and in the installation
    add_custom_command(TARGET ${target} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different
                               $<TARGET_FILE:${target}> $<TARGET_FILE_DIR:${target}>/${alias_file}
                       VERBATIM)
    install(PROGRAMS $<TARGET_FILE:${target}> DESTINATION bin RENAME ${alias_file})
    return()
  endif()

  add_custom_command(TARGET ${target} POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E create_symlink ${file} $<TARGET_FILE_DIR:${target}>/${alias_file}
                     VERBATIM)
  _medio_install_link("${alias_file}" "${file}")
  if(MEDIO_INSTALL_TOOL_LINKS AND suffix)
    _medio_install_link("${alias}" "${alias_file}")
  endif()
endfunction()
