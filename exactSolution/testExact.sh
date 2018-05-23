#!/bin/bash
rm -f "out$1.$2.$3.log"
for i in `seq $1 $2`
do
	./testExactInGen.py $i $3 > tmpInput.xml
	for j in `seq 1 $4`
	do
		./exactSolution.sh tmpInput.xml "out$1.$2.$3.log" $i
	done
done
rm -f tmpInput.xml
