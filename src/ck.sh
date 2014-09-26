#!/bin/bash -e

for file in `ls`
do
	echo ${file}
	sed -n '1,10p' ${file}
done
