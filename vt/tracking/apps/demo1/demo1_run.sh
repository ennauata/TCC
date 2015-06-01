#!/bin/bash          

make -C ../../ clean
make -C ../../ 

make -C ./ clean
make -C ./

./demo1 $1 $2
