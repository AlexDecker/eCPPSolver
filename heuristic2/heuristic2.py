import random
import sys
sys.path.insert(0,'../utils')
import eCPPGraph
import eCPPDominantingSet
import evaluateSolution

def run(graph):
	#if all bad edges (whose latency is more than this threshold)
	#were removed, the solution is certainly feasible
	if len(graph.nodeList)>1:
		threshold = graph.maxTotalLatency/(len(graph.nodeList)-1)
	else:
		#there is no edge, so this value is irrelevant
		threshold = 0

	#lists of edges from eCPPGraph
	blackList = []
	badEdgeList = []
	for n in graph.nodeList:
		for e in n._neighborhood:
			if e.latency>threshold:
				badEdgeList.append(e)	

	while True:
		#prepare the new subgraph for execution
		graph.makeSandBox()
		#eliminate some bad edges
		for e in blackList:
			graph.nodeList[e.fromNode].neighborhood_sandbox.remove(e)
			graph.nodeList[e.toNode].neighborhood_sandbox.remove(\
				e.returnEdge)
		
		placementVector,assignMatrix = eCPPDominantingSet.solve(\
			graph)
		value,feasible = evaluateSolution.eval(placementVector,\
			assignMatrix,graph)
		if not feasible:
			#assume it is bacause of the latency constraint
			if badEdgeList!=[]:
				e = random.choice(badEdgeList)
				badEdgeList.remove(e)
				try:
					badEdgeList.remove(e.returnEdge)
				except ValueError:
					pass #just ignore
				blackList.append(e)
			else:
				print 'Something went wrong'
				return value, False
		else:
			break
	return value, feasible, placementVector, assignMatrix

def test(file = '../testScripts/autoGen.xml', nSFreq = 10,\
	nSLat = 10):
	while True:
		g = eCPPGraph.graph(file,\
			nSamplesFreq=nSFreq,nSamplesLat=nSLat)
		
		value,feasible,_,_ = run(g)
		if feasible:
			print value
		else:
			exit()
