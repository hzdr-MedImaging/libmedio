#!/bin/bash
#
# Starts the readwritematrix test with
# the correct LD_LIBRARY_PATH

export LD_LIBRARY_PATH=../../lib:/usr/local/petlib/lib:$LD_LIBRARY_PATH
./readwritematrix
