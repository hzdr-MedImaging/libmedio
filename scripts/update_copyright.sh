#!/bin/sh
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
# simply bumps the year in all copyright and header files to the latest one
#
# WARNING: This script will run over ALL files regardless if they are text
# or binary files and search for the copyright notice and replace it
# immediately. So you need to be aware of that
#

YEAR=$(date +%Y)

# walk through the whole directory this script is called in and search
# for files which we will try to update the Copyright notice
find . \( -not -path "*/.git/*" -not -name "update_copyright.sh" \) -type f \
       -exec sed -i "s/Copyright.*(C).*200.-.* hzdr.de and contributors/Copyright (C) 2004-${YEAR} hzdr.de and contributors/g" {} \;

