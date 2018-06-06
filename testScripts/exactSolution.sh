#!/bin/bash

#Usage: exactSolution.sh [xml_file] [log_file] [experiment_id]
../exactSolution/lsGenerator.py $1 > outputLsGenerator.lp
startTime=`date +%s.%N`
glpsol --cpxlp outputLsGenerator.lp -o outputGLPK.txt >/dev/null
endTime=`date +%s.%N`
./logger.py $2 $startTime $endTime $3< outputGLPK.txt
rm -f outputLsGenerator.lp
rm -f outputGLPK.txt
