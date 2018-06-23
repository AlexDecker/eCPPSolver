import random
import sys
sys.path.insert(0,'../utils')
import eCPPGraph
import capacitedDominantingSet
import evaluateSolution

def run(g):
	#if all bad edges (whose latency is more than this threshold)
	#were removed, the solution is certainly feasible
	if len(g.nodeList)>1:
		threshold = g.maxTotalLatency/(len(g.nodeList)-1)
	else:
		#there is no edge, so this value is irrelevant
		threshold = 0

	#lists of edges from eCPPGraph
	blackList = []
	badEdgeList = []
	for n in g.nodeList:
		for e in n._neighborhood:
			if e.latency>threshold:
				badEdgeList.append(e)	

	while True:
		#prepare the new subgraph for execution
		g.makeSandBox()
		#eliminate some bad edges
		for e in blackList:
			g.nodeList[e.fromNode].neighborhood_sandbox.remove(e)
			g.nodeList[e.toNode].neighborhood_sandbox.remove(e.returnEdge)
		
		s,a = capacitedDominantingSet.solve(g)
		value,feasible = evaluateSolution.eval(s,a,g)
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
	return value, feasible

def test(nSFreq, nSLat):
	while True:
		g = eCPPGraph.graph('../testScripts/autoGen.xml',\
			nSamplesFreq=nSFreq,nSamplesLat=nSLat)
		value,feasible = run(g)
		if feasible:
			print value
		else:
			exit()