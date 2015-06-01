#!/bin/bash          

for d in ./weizmann_dataset/*/ ; do

	prefix="./weizmann_dataset/"
	sulfix="/"
	dirname=${d#$prefix}
	dirname=${dirname%$sulfix}
	it=0
    for fname in $d*.avi ; do
    	mkdir "./seq_files/$dirname/train$it"
    	sh demo1_run.sh $fname "$dirname/train$it"
    	((it++))
	done
done