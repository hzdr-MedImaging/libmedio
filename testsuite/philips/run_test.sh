#!/bin/bash
#
# Starts the readwritematrix test with
# the correct LD_LIBRARY_PATH

if [ $# -ne 1 ]; then
    echo "usage:" $0 "<filename>"
    exit 2
fi

export LD_LIBRARY_PATH=../../lib:/usr/local/petlib/lib:$LD_LIBRARY_PATH
./readmainheader $1
