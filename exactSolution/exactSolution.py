#(needs GLPK)
import os
import lpGenerator

import sys
sys.path.insert(0,'../utils')

import eCPPGraph
import evaluateSolution

def run(graph):
	placementVector = [0 for n in graph.nodeList]
	assignMatrix = [[0 for n in graph.nodeList]\
		for n in graph.nodeList]
	
	#generates the integer linear programming model
	lp = lpGenerator.gen(graph)
	
	#creates the intermediate file for the glpsol call
	with open('eCPPInput.lp', 'w') as myfile:
		myfile.write(lp)
		myfile.close()
	
	#call glpsol synchronously
	os.popen('glpsol --cpxlp eCPPInput.lp -o outputGLPK.txt >/dev/null')
	
	#read the variables
	with open('outputGLPK.txt', 'r') as myfile:
		while(True):
			line = myfile.readline()
			if line=='End of output':
				break
			#TODO: completar corretamente as variáveis
	
	value,feasible = evaluateSolution.eval(placementVector,\
		assingMatrix, graph)
	
	return value, feasible, placementVector, assignMatrix