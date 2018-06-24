import constraints
import objective

import sys
sys.path.insert(0,'../utils')

import eCPPGraph

def gen(graph):
	ret = ''
	ret = ret + objective.generateObjective(graph.costList,graph.cPower,graph.adjMatrix,\
				graph.energy,graph.cProcEnergy,graph.sFreq)
	
	ret = ret + constraints.generateConstraints(graph.adjMatrix,graph.sFreq,graph.cFreq,\
				graph.latencyMatrix,graph.maxTotalLatency)
	
	ret = ret + 'Binary\n'
	for i in range(len(graph.adjMatrix)):
		for j in range(len(graph.adjMatrix)):
			if(graph.adjMatrix[i][j]==1):
				ret = ret + '\te' + str(i) + '_' + str(j) + '\n'
	
	for i in range(len(graph.adjMatrix)):
		ret = ret + '\tp' + str(i) + '\n'
	
	return(ret+'End')

def test(file = '../testScripts/autoGen.xml', nSFreq = 10,\
	nSLat = 10):
	graph = eCPPGraph.graph(file,nSamplesFreq=nSFreq,nSamplesLat=nSLat)
	print gen(graph)