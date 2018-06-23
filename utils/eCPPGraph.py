import numpy

import inputParser #input informed by the user
import inGen #randomly generated input

#edges representing the control links
class edge:
	#-latency: this is called latency just for reasons of consistency, but is the
	#expected RTT for a Request+Response, considering the processing time and
	#the propagation time
	#-weight: the energy spent with requst+responses between those two nodes
	#-fromNode: id of the node from which the edge comes (only in order to make
	#this class independent)
	#-toNode: id of the node to which the edge goes
	
	
	def __init__(self,latency,weight,fromNode,toNode):
		self.latency = latency
		self.weight = weight
		self.fromNode = fromNode
		self.toNode = toNode
		self.returnEdge = None
	
	def printEdge(self):
		print '\t\t(Edge to #'+str(self.toNode)+') Latency: '+str(self.latency)+\
			', Weight: '+str(self.weight)

#node representing a location of the original problem
class node:
	#-demand: the same of sFreq (how much requests are sent for each time interval)
	#-capacity: the same of cFreq (how much responses can be sent for each time interval)
	#-weight: the energy needed to maintain itself as a controller (not considering outter links)
	#-_neighborhood: the locations connected to this one by control links (List of edges)
	#-neighborhood_sandbox: this can be freely altered without losing data
	
	def __init__(self,demand,capacity,weight):
		self.demand = demand
		self.capacity = capacity
		self.weight = weight
		self._neighborhood = []
		self.neighborhood_sandbox = []
	
	def addEdge(self,edge):
		self._neighborhood.append(edge)
	
	def makeSandBox(self):
		self.neighborhood_sandbox = list(self._neighborhood)
	
	def printNode(self):
		print '\tDemand: '+str(self.demand)+', Capacity: '+str(self.capacity)\
			+', Weight: '+str(self.weight)
		for e in self._neighborhood:
			e.printEdge()
	
	def printCurrNode(self):
		print '\tDemand: '+str(self.demand)+', Capacity: '+str(self.capacity)\
			+', Weight: '+str(self.weight)
		for e in self.neighborhood_sandbox:
			e.printEdge()

#symmetrical graph
class graph:
	#this class has both matricial and graph representations. Each representation
	#is useful for some reason in terms of efficiency
	def __init__(self,filename,nSamplesFreq=10,nSamplesLat=10):
		self.nodeList = []
		#read file and get/generate the parameters
		inst = inputParser.eCPPInstance(filename)
		self.maxTotalLatency = inst.getMaxTotalLatency()
		self.adjMatrix, self.latencyMatrix, self.energy = inst.getAdjMatrices()
		self.sFreq, self.cFreq = inst.getFrequencies()
		self.costList = inst.getCostList()
		self.sProcEnergy, self.cProcEnergy = inst.getProcEnergy()
		self.cPower, self.sPower = inst.getStaticPower()
		
		if(self.cFreq==-1):
			self.cFreq = inGen.genCFreq(self.adjMatrix,self.sFreq,\
				nSamplesFreq)
		if(self.maxTotalLatency==-1):
			self.maxTotalLatency = inGen.genMaxTotalLatency(self.adjMatrix,\
				self.latencyMatrix, nSamplesLat)
		
		warning=False
		for i in range(len(self.adjMatrix)):
			for j in range(len(self.adjMatrix)):
				if self.adjMatrix[i][j] != self.adjMatrix[j][i]:
					self.adjMatrix[i][j] = self.adjMatrix[j][i]
					warning=True
		if warning:
			print 'Warning: adjMatrix modified to became symmetrical'
		
		#organize the parameters more efficiently
		for i in range(len(self.adjMatrix)):
			#energy needed by the controller to process the requests from this switch
			Eproc = self.sFreq[i]*self.cProcEnergy
			
			#cost for keeping the controller here (without constant_costs)
			weight = self.costList[i]*(self.cPower+Eproc)
			
			n = node(self.sFreq[i],self.cFreq,weight)
			
			for j in range(len(self.adjMatrix)):
				if((self.adjMatrix[i][j]==1) and (i!=j)):
					#cost for keeping connected with the controller in j
					weight = self.costList[i]*self.sFreq[i]*self.energy[i][j]\
						+self.costList[j]*(Eproc+self.sFreq[i]*self.energy[j][i])
					n.addEdge(edge(self.latencyMatrix[i][j],weight,i,j))
			
			self.nodeList.append(n)
		
		self._fillReturnEdges()
	
	def printOriginalGraph(self):
		print 'printing the original graph for this instance...\n'
		for i in range(len(self.nodeList)):
			print 'Node #'+str(i) 
			self.nodeList[i].printNode()
	
	def printCurrentGraph(self):
		print 'printing the current graph for this instance...\n'
		for i in range(len(self.nodeList)):
			print 'Node #'+str(i) 
			self.nodeList[i].printCurrNode()
	
	def makeSandBox(self):
		for n in self.nodeList:
			n.makeSandBox()
	
	def sortEdgesByDemand(self):
		for n in self.nodeList:
			n.neighborhood_sandbox.sort(key=lambda e:self.nodeList[e.toNode].demand,\
				reverse = True) 
	
	#put a reference to (i,j) in every (j,i)
	def _fillReturnEdges(self):
		for n in self.nodeList:
			for e1 in n._neighborhood:
				for e2 in self.nodeList[e1.toNode]._neighborhood:
					if e2.toNode == e1.fromNode:
						e1.returnEdge = e2