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
		if domSet[node]==0:
			x = g.nodeList[node].demand #it must include its own demand
			#cost for just keeping the controller here (without 
			#constant_costs)
			cost = g.nodeList[node].weight 
		else:
			x = 0 #its demant yet have been included
			cost = 0
		
		for j in range(i):
			x = x + g.nodeList[\
				g.nodeList[node].neighborhood_sandbox[j].toNode\
				].demand
			cost = cost + g.nodeList[node].neighborhood_sandbox[j].weight
		
		#if the assingnment is feasible
		if x<=g.nodeList[node].capacity:
			if (x==0) or (cost==0):
				ratio = float('inf') #the ratio becames naturally inf
			else:
				ratio = x/cost
			if ratio>maxRatio:#selects the best ration seen so far
				maxRatio = ratio
				maxIndex = i
			i = i+1
		else:
			#no other will be feasible (the demands will only increase)
			break
	
	return maxRatio,maxIndex

def mostEfficientNode(g,domSet,notDom):
	maxEff = 0
	maxIndex = 0
	bestNode = 0
	for i in range(len(g.nodeList)):
		eff,ind = efficiency(g,i,domSet)
		if(eff>=maxEff):#if all are 0, any node can be returned
			if(eff!=0):#normal condition
				maxEff = eff
				maxIndex = ind
				bestNode = i
			elif(i in notDom):
				#prefer the ones that still are not dominated
				maxEff = eff
				maxIndex = ind
				bestNode = i
	return bestNode,maxIndex

def solve(g):
	g.sortEdgesByDemand()
	
	notDom = [i for i in range(len(g.nodeList))] #undominated vertices
	domSet = [0 for i in range(len(g.nodeList))] #dominating set
	
	#the bit map matrix that says if node i is dominated by node j
	assign = [[0 for j in range(len(g.nodeList))]\
		 for i in range(len(g.nodeList))]
	
	while notDom != []:
		#get the most efficient vertex
		u,ind = mostEfficientNode(g,domSet,notDom)
		
		#insert it into the dominating set
		if domSet[u]==0:
			assign[u][u] = 1
			domSet[u]=1
			#it must fullfil its own demand
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
		
		neighbors = g.nodeList[u].neighborhood_sandbox[:ind]
		
		for neighbor in neighbors:
			n = neighbor.toNode
			
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
