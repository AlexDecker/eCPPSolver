from xml.dom import minidom

#energy oriented controller placement problem instance
class eCPPInstance:

	#the function below eliminates the out-of-bounds-latency
	#and returns the matrix of latency
	def _eliminateLargeLatency(self,adjMatrix,propTimeMatrix,\
		sProcTimeVector,cProcTime):
		
		#building an empty answer
		latencyMatrix = [[0 for i in adjMatrix] for j in adjMatrix]
		
		#for each switch
		for i in range(len(adjMatrix)):
			#for each possible location of a controller
			for j in range(len(adjMatrix[i])):
				#if the switch/controller edge exists
				if adjMatrix[i][j]:
					latencyMatrix[i][j] = cProcTime+propTimeMatrix[i][j]\
						+propTimeMatrix[j][i]+sProcTimeVector[i]
					#if the latency exceeds the maximum
					if latencyMatrix[i][j]>self._maxSingleLatency:
						#disconsider this edge
						adjMatrix[i][j]=0
		
		return latencyMatrix, adjMatrix
	
	#the total energy spent by a single request+response (only controller's side)
	def _getTotalEnergyMatrix(self,adjMatrix,energyMatrix):
		
		cProcEnergy = float(self.controller.attributes['procEnergy'].value)
		
		totalEnergyMatrix = [[0 for i in adjMatrix] for j in adjMatrix]
		
		#for each switch
		for i in range(len(adjMatrix)):
			#for each possible location of a controller
			for j in range(len(adjMatrix[i])):
				#if the switch/controller edge exists
				#the energy cost for the controller is its constant energy spent
				#for processing a request plus the cost of sending a message back
				#(from the controller j to the switch i)
				if adjMatrix[i][j]:
					totalEnergyMatrix[i][j] = cProcEnergy+energyMatrix[j][i];
		
		return totalEnergyMatrix
	
	def __init__(self, fileName):
		
		data = minidom.parse(fileName)
		self.nodeList = data.getElementsByTagName('node')
		self.edgeList = data.getElementsByTagName('edge')
		
		controller = data.getElementsByTagName('controller')
		self.controller = controller[0]
		
		maxTotalLatency = data.getElementsByTagName('maxTotalLatency')
		self.maxTotalLatency = float(maxTotalLatency[0].attributes['value'].value)
		
		maxSingleLatency_tagList = data.getElementsByTagName('maxSingleLatency')
		self._maxSingleLatency = float(maxSingleLatency_tagList[0].attributes['value'].value)
	
	#maximum value for the sum of the latency of each used connection
	def getMaxTotalLatency(self):
		return self.maxTotalLatency
	
	#get:
	#*the possible control connections between
	#each pair of switch locations (adjMatrix)
	#*the RTT time between in each valid
	#link for one request + response (latencyMatrix)
	#*the energy cost for one request + response in
	#each link (totalEnergyMatrix)
	def getAdjMatrices(self):
		
		nodeNum = len(self.nodeList)
		
		adjMatrix = [[0 for j in range(nodeNum)]for i in range(nodeNum)]
		for i in range(nodeNum):
			adjMatrix[i][i] = 1 #there is always a connection to itself
		
		#auxiliar variables
		energyMatrix = [[0 for j in range(nodeNum)]for i in range(nodeNum)]
		propTimeMatrix = [[0 for j in range(nodeNum)]for i in range(nodeNum)]
		sProcTimeVector = []
		for n in self.nodeList:
			sProcTimeVector.append(float(n.attributes['procTime'].value))
		cProcTime = float(self.controller.attributes['procTime'].value)
		
		#building the matrices
		for e in self.edgeList:
			node1 = int(e.attributes['node1'].value)
			node2 = int(e.attributes['node2'].value)
			
			if node1>=nodeNum or node1<0 or node2>=nodeNum or node2<0:
				print 'Warning: invalid connection: '+str(node1)+' to '+str(node2)
			else:
				adjMatrix[node1][node2] = 1
				adjMatrix[node2][node1] = 1
				propTimeMatrix[node1][node2] = float(e.attributes['propTime'].value)
				energyMatrix[node1][node2] = float(e.attributes['energyCost'].value)
				#default values for the returning link
				if propTimeMatrix[node2][node1] == 0:
					propTimeMatrix[node2][node1] = propTimeMatrix[node1][node2]
					
				if energyMatrix[node2][node1] == 0:
					energyMatrix[node2][node1] = energyMatrix[node1][node2]
		
		latencyMatrix,adjMatrix = self._eliminateLargeLatency(adjMatrix,propTimeMatrix,\
									sProcTimeVector,cProcTime)
		totalEnergyMatrix = self._getTotalEnergyMatrix(adjMatrix,energyMatrix)
		
		return adjMatrix, latencyMatrix, totalEnergyMatrix
	
	#returns a list sfreq of the request frequencies of each switch and a float
	#cfreq with the maximum response frequency of the controller
	def getFrequencies(self):
		cfreq = float(self.controller.attributes['freq'].value)
		sfreq = []
		for n in self.nodeList:
			sfreq.append(float(n.attributes['freq'].value))
		
		return sfreq, cfreq
	
	#returnes the financial cost per joule (Ws)
	#(assuming the message frequency is in hertz) for each location
	def getCostList(self):
		costList = []
		for n in self.nodeList:
			costList.append(float(n.attributes['cost'].value))
		return costList

