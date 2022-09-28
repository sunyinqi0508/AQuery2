#!/usr/bash
module load g++-11.2
PWD=`pwd`
export LD_LIBRARY_PATH=$PWD/usr/lib64:$LD_LIBRARY_PATH:/lib:/lib64:/usr/lib:/usr/lib64
export AQ_MONETDB_LIB=-L$PWD/usr/lib64
export AQ_MONETDB_INC=-I$PWD/usr/include/monetdb/
export CXX=g++-11.2
export PCH=1 # Change to 0
