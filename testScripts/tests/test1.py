#!/usr/bin/env python

import sys

sys.path.insert(0,'../exactSolution')
import exactSolution

sys.path.insert(0,'../heuristic1')
import heuristic1

sys.path.insert(0,'../heuristic2')
import heuristic2

sys.path.insert(0,'../heuristic3')
import heuristic3

sys.path.insert(0,'../utils')
import eCPPGraph

def generateInput():
	

generatedInput = generateInput(



graph = eCPPGraph.graph('generatedInput.xml')


valueExact, feasible, _, _ = exactSolution.run(graph)
if not feasible:
	print 'Not feasible solution found using the exact solution.'
	exit()
print valueExact

valueHeu1, feasible, _, _ = heuristic1.run(graph)
if not feasible:
	print 'Not feasible solution found using heuristic1.'
	exit()
print valueHeu1

valueHeu2, feasible, _, _ = heuristic2.run(graph)
if not feasible:
	print 'Not feasible solution found using heuristic2.'
	exit()
print valueHeu2

valueHeu3, feasible, _, _ = heuristic3.run(graph)
if not feasible:
	print 'Not feasible solution found using heuristic3.'
	exit()
print valueHeu3
