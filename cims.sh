#!/usr/bash
echo  "Don't execute this script if it's not on CIMS servers."
echo "run this script with source command. e.g. \`source ./cims.sh\` or \`. ./cims.sh\`"
module load g++-11.2
PWD=`pwd`
export LD_LIBRARY_PATH=$PWD/usr/lib64:$LD_LIBRARY_PATH:/lib:/lib64:/usr/lib:/usr/lib64
export AQ_MONETDB_LIB=-L$PWD/usr/lib64
export AQ_MONETDB_INC=-I$PWD/usr/include/monetdb/
export CXX=g++-11.2
export PCH=1 # Change to 0
