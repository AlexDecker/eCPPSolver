#!/bin/bash
./lsGenerator.py $1 > outputLsGenerator.lp
glpsol --cpxlp outputLsGenerator.lp -o outputGLPK.txt >/dev/null
./logger.py $2 < outputGLPK.txt
rm -f outputLsGenerator.lp
rm -f outputGLPK.txt
