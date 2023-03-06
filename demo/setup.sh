#!/bin/sh 

make -C ../sdk irf
mkdir ../procedures 
cp demo*.aqp ../procedures 
make 
