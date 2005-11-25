#!/bin/sh
#
# Starts the readwritematrix test with
# the correct LD_LIBRARY_PATH

LD_LIBRARY_PATH=../../lib:$LD_LIBRARY_PATH
./readwritematrix
