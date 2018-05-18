#!/bin/bash
./lsGenerator.py $1 > out.lp
glpsol --cpxlp out.ls -o $2
