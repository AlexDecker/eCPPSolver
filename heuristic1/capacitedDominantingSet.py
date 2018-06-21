import math
import sys
sys.path.insert(0,'../utils')
import eCPPGraph

#This function finds a dominating set for eCPPGraph g that respects
#capacities and inseparable demands. It minimizes the sum of the weights
#of the vertices with a (ln n)-Approximation. Each node must be able to
#satisfy its own demand if included in the dominanting set.

#Adapted and extended from:
#("Capacitated Domination: Problem Complexity and Approximation Algorithms"),
#Kao et al.

def efficiency(g,node,domSet):
	maxRatio = 0 #0 when the neighborhood is empty
	maxIndex = 0
	i=1
	
	if g.nodeList[node].capacity==0:
		return 0,0 #no capacity -> no efficiency
	
	for e in g.nodeList[node].neighborhood_sandbox:
		if node not in domSet:
			x = g.nodeList[node].demand #it must include its own demand
		else:
			x = 0 #its demant yet have been included
		
		for j in range(i):
			x = x + g.nodeList[\
				g.nodeList[node].neighborhood_sandbox[j].toNode\
				].demand
		
		#if the assingnment is feasible
		if x<=g.nodeList[node].capacity:
			if (x==0) or (g.nodeList[node].weight==0):
				ratio = float('inf') #the ratio becames naturally inf
			else:
				ratio = i/g.nodeList[node].weight
			if ratio>maxRatio:#selects the best ration seen so far
				maxRatio = ratio
				maxIndex = i
			i = i+1
		else:
			#no other will be feasible (the demands will only increase)
			break
	
	return maxRatio,maxIndex

def mostEfficientNode(g,domSet):
	maxEff = 0
	maxIndex = 0
	bestNode = -1
	for i in range(len(g.nodeList)):
		eff,ind = efficiency(g,i,domSet)
		if(eff>=maxEff):#if all are 0, any node can be returned
			if(eff!=0):#normal condition
				maxEff = eff
				maxIndex = ind
				bestNode = i
			elif(i not in domSet):
				#prefer the ones which still are not in dominanting set
				maxEff = eff
				maxIndex = ind
				bestNode = i
	return bestNode,maxIndex

def solve(g):
	g.makeSandBox()
	g.sortEdgesByDemand()
	
	notDom = [i for i in range(len(g.nodeList))] #undominated vertices
	domSet = [] #dominating set
	
	#the bit map matrix that says if node i is dominated by node j
	assign = [[0 for j in range(len(g.nodeList))]\
		 for i in range(len(g.nodeList))]
	
	while notDom != []:
		#get the most efficient vertex
		u,ind = mostEfficientNode(g,domSet)
		
		#insert it into the dominating set
		if u not in domSet:
			assign[u][u] = 1
			domSet.append(u)
			g.nodeList[u].capacity = g.nodeList[u].capacity\
				-g.nodeList[u].demand
			#for each neighbor
			for e in g.nodeList[u]._neighborhood:
				#remove u from the adjacence list of the neighbor
				g.nodeList[e.toNode].neighborhood_sandbox = [\
					x for x in g.nodeList[e.toNode].neighborhood_sandbox\
						if x.toNode!=u]
				#if it was assigned to another vertex, reincrement its
				#remaining capacity
				if assign[u][e.toNode]==1:
					assign[u][e.toNode] = 0
					g.nodeList[e.toNode].capacity = \
						g.nodeList[e.toNode].capacity+g.nodeList[u].demand
		
		#try to remove it from the undomined vertex set
		#(it may have already been removed)
		try:
			notDom.remove(u)
		except ValueError:
			pass #just ignore
		
		for i in range(ind):
			n = g.nodeList[u].neighborhood_sandbox[i].toNode
			
			if n in notDom:
				#remove the neighbor from the undomined vertex set
				notDom.remove(n)
				
				#assing the nodes
				g.nodeList[u].capacity = g.nodeList[u].capacity\
					-g.nodeList[n].demand
				assign[n][u] = 1
				
				#for each neighbor e of the neighbor n
				for e in g.nodeList[n]._neighborhood:
					#remove n from the adjacency list of e
					g.nodeList[e.toNode].neighborhood_sandbox = [\
						x for x in g.nodeList[e.toNode].neighborhood_sandbox\
							if x.toNode!=n]
			else:
				print 'Error: '+str(n)+' is not in '+str(notDom)
		
	return domSet, assign
