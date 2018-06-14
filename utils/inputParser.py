from xml.dom import minidom
import inGen
import random

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
	
	def __init__(self, fileName):
		data = minidom.parse(fileName)
		config = data.getElementsByTagName('settings')
		self.autoGen = config[0].attributes['autoGen'].value=='True'
		if(self.autoGen):
			self.nNodes = int(config[0].attributes['nNodes'].value)
			self.nEdges = int(config[0].attributes['nEdges'].value)
			
			self.minLon = float(config[0].attributes['minLongitude'].value)
			self.maxLon = float(config[0].attributes['maxLongitude'].value)
			self.minLat = float(config[0].attributes['minLatitude'].value)
			self.maxLat = float(config[0].attributes['maxLatitude'].value)
			
			#Km/s
			self.minC = float(config[0].attributes['minLightSpeed'].value)
			self.maxC = float(config[0].attributes['maxLightSpeed'].value)
			
			self.minCProcTime = float(config[0].attributes['minControllerPacketProcessingTime'].value)
			self.maxCProcTime = float(config[0].attributes['maxControllerPacketProcessingTime'].value)
			self.minSProcTime = float(config[0].attributes['minSwitchPacketProcessingTime'].value)
			self.maxSProcTime = float(config[0].attributes['maxSwitchPacketProcessingTime'].value)
			
			self.minE = float(config[0].attributes['minEnergyPerBitPerKm'].value)
			self.maxE = float(config[0].attributes['maxEnergyPerBitPerKm'].value)
			
			self.avgMsgSize = float(config[0].attributes['avgMsgSize'].value)
			
			self.minSFreq = float(config[0].attributes['minSwitchRequestFrequency'].value)
			self.maxSFreq = float(config[0].attributes['maxSwitchRequestFrequency'].value)
			
			self.minCost = float(config[0].attributes['minCost'].value)
			self.maxCost = float(config[0].attributes['maxCost'].value)
			
			self.minControllerStaticPower = float(config[0].attributes['minControllerStaticPower'].value)
			self.maxControllerStaticPower = float(config[0].attributes['maxControllerStaticPower'].value)
			
			self.minSwitchStaticPower = float(config[0].attributes['minSwitchStaticPower'].value)
			self.maxSwitchStaticPower = float(config[0].attributes['maxSwitchStaticPower'].value)
			
			self.minCProcEnergy = float(\
				config[0].attributes['minControllerPacketProcessingEnergy'].value)
			self.maxCProcEnergy = float(\
				config[0].attributes['maxControllerPacketProcessingEnergy'].value)
			self.minSProcEnergy = float(\
				config[0].attributes['minSwitchPacketProcessingEnergy'].value)
			self.maxSProcEnergy = float(\
				config[0].attributes['maxSwitchPacketProcessingEnergy'].value)
		else:
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
		if(self.autoGen):
			return -1
		else:
			return self.maxTotalLatency
	
	#get:
	#*the possible control connections between
	#each pair of switch locations (adjMatrix)
	#*the RTT time between in each valid
	#link for one request + response (latencyMatrix)
	#*the energy cost for one request + response in
	#each link (totalEnergyMatrix)
	def getAdjMatrices(self):
		if(self.autoGen):
			adjMatrix = inGen.genRandomAdjMatrix(self.nNodes,self.nEdges)
			
			distMatrix = inGen.genRandomDistMatrix(adjMatrix,self.minLon,\
				self.maxLon,self.minLat,self.maxLat)
				
			cMatrix = [[random.uniform(self.minC,self.maxC)\
				for i in range(self.nNodes)] for i in range(self.nNodes)]
			sProcTime = [random.uniform(self.minSProcTime,self.maxSProcTime)\
				for i in range(self.nNodes)]
			cProcTime = random.uniform(self.minCProcTime,self.maxCProcTime)
			
			latencyMatrix = inGen.genLatencyMatrix(cMatrix,distMatrix,\
				sProcTime,cProcTime)
			
			eMatrix = [[random.uniform(self.minE,self.maxE)\
				for i in range(self.nNodes)] for i in range(self.nNodes)]
			
			energyMatrix = inGen.genEnergyMatrix(eMatrix,distMatrix,self.avgMsgSize)
		else:
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
		return adjMatrix, latencyMatrix, energyMatrix
	
	#returns a list sfreq of the request frequencies of each switch and a float
	#cfreq with the maximum response frequency of the controller
	def getFrequencies(self):
		if(self.autoGen):
			cfreq = -1
			sfreq = [random.uniform(self.minSFreq,self.maxSFreq)\
				for i in range(self.nNodes)]
		else:
			cfreq = float(self.controller.attributes['freq'].value)
			sfreq = []
			for n in self.nodeList:
				sfreq.append(float(n.attributes['freq'].value))
		
		return sfreq, cfreq
	
	#returnes the financial cost per joule (Ws)
	#(assuming the message frequency is in hertz) for each location
	def getCostList(self):
		if(self.autoGen):
			costList = [random.uniform(self.minCost,self.maxCost)\
				for i in range(self.nNodes)]
		else:
			costList = []
			for n in self.nodeList:
				costList.append(float(n.attributes['cost'].value))
		return costList
	
	#returns the power of the controller when not processing any response
	#and the power of each switch when not processing any frame
	def getStaticPower(self):
		if(self.autoGen):
			cPower = random.uniform(self.minControllerStaticPower,self.maxControllerStaticPower)
			sPower = [random.uniform(self.minSwitchStaticPower,self.maxSwitchStaticPower)\
				for i in range(self.nNodes)]
		else:
			cPower = float(self.controller.attributes['staticPower'].value)
			sPower = []
			for n in self.nodeList:
				sPower.append(float(n.attributes['staticPower'].value))
		return cPower, sPower
	
	#returns the energy spent by the controller (cProcEnergy) for processing
	#one response and the energy spent by each switch (sProcEnergy) for
	#processing one request
	def getProcEnergy(self):
		if(self.autoGen):
			cProcEnergy = random.uniform(self.minCProcEnergy,self.maxCProcEnergy)
			sProcEnergy = [random.uniform(self.minSProcEnergy,self.maxSProcEnergy)\
				for i in range(self.nNodes)]
		else:
			cProcEnergy = float(self.controller.attributes['procEnergy'].value)
			sProcEnergy = []
			for n in self.nodeList:
				sProcEnergy.append(float(n.attributes['procEnergy'].value))
		
		return sProcEnergy, cProcEnergy
