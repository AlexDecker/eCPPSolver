import numpy

import inputParser #input informed by the user
import inGen #randomly generated input

#edges representing the control links
class edge:
	#-latency: this is called latency just for reasons of consistency, but is the
	#expected RTT for a Request+Response, considering the processing time and
	#the propagation time
	#-weight: the energy spent with requst+responses between those two nodes
	#-toNode: id of the node to which the edge goes
	
	
	def __init__(self,latency,weight,toNode):
		self.latency = latency
		self.weight = weight
		self.toNode = toNode
	
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

class graph:
	
	def __init__(self,filename,nSamples=10):
		self.nodeList = []
		#read file and get/generate the parameters
		inst = inputParser.eCPPInstance(filename)
		self.maxTotalLatency = inst.getMaxTotalLatency()
		adjMatrix, latencyMatrix, energy = inst.getAdjMatrices()
		sFreq, cFreq = inst.getFrequencies()
		costList = inst.getCostList()
		_,cProcEnergy = inst.getProcEnergy()
		cPower,_ = inst.getStaticPower()
		if(cFreq==-1):
			cFreq = inGen.genCFreq(adjMatrix,sFreq,nSamples)
		if(self.maxTotalLatency==-1):
			self.maxTotalLatency = inGen.genMaxTotalLatency(adjMatrix,\
				latencyMatrix,nSamples)
		
		#organize the parameters more efficiently
		for i in range(len(adjMatrix)):
			#energy needed by the controller to process the requests from this switch
			Eproc = sFreq[i]*cProcEnergy
			
			#cost for keeping the controller here (without constant_costs)
			weight = costList[i]*(cPower+Eproc)
			
			n = node(sFreq[i],cFreq,weight)
			
			for j in range(len(adjMatrix)):
				if((adjMatrix[i][j]==1) and (i!=j)):
					#cost for keeping connected with the controller in j
					weight = costList[i]*sFreq[i]*energy[i][j]\
						+costList[j]*(Eproc+sFreq[i]*energy[j][i])
					n.addEdge(edge(latencyMatrix[i][j],weight,j))
			
			self.nodeList.append(n)
	
	def printOriginalGraph(self):
		print 'printing the original graph for this instance...\n'
		for i in range(len(self.nodeList)):
			print 'Node #'+str(i) 
			self.nodeList[i].printNode()
	
	def printCurrentGraph(self):
		print 'printing the original graph for this instance...\n'
		for i in range(len(self.nodeList)):
			print 'Node #'+str(i) 
			self.nodeList[i].printCurrNode()
