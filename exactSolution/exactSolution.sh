#!/bin/bash
./lsGenerator.py $1 > outputLsGenerator.lp
glpsol --cpxlp outputLsGenerator.lp -o $2
rm -f outputLsGenerator.lp
