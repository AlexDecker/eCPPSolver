#(tested with GLPK )
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
	
	#read the variables (using a different version of GLPK may break this part)
	feasible = False
	with open('outputGLPK.txt', 'r') as myfile:
		while(True):
			line = myfile.readline()
			if line=='':
				break
			elif 'INTEGER OPTIMAL' in line:
				feasible = True
			else:
				line_l = line.split()
				if len(line_l)==6:
					#Valid forms: '[int] p[int] * [val] [int] [int]'
					#			  '[int] e[int]_[int] * [val] [int] [int]'
					if(line_l[1][0]=='p'):
						placementVector[int(line_l[1][1:])] = int(line_l[3])
					elif(line_l[1][0]=='e'):
						coordinates = line_l[1][1:].split('_')
						assignMatrix[int(coordinates[0])][int(coordinates[1])] = \
							int(line_l[3])
	
	if feasible:
		value,feasible = evaluateSolution.eval(placementVector,\
			assignMatrix, graph)
	else:
		value = float('inf')
		feasible = False
	
	return value, feasible, placementVector, assignMatrix

def test(file = '../testScripts/autoGen.xml', nSFreq = 10,\
	nSLat = 10):
	
	g = eCPPGraph.graph(file,\
		nSamplesFreq=nSFreq,nSamplesLat=nSLat)
	
	value,feasible,_,_ = run(g)
	if feasible:
		print value
	else:
		exit()
