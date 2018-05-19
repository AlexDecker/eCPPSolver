#this file has two functions that validate some configuration of both
#placementVector (a binary vector that points what locations have a
#controller) and the connectionMatrix (a binary matrix that points if
#switch i has a control connection with the controller at location j)
#and evaluates its energy consumption

def isFeasible(placementVector, connectionMatrix, adjMatrix,\
	latencyMatrix,sFreq,cFreq,maxTotalLatency):
	for i in range(len(placementVector)):
		connections = 0
		for j in range(len(placementVector)):
			if(connectionMatrix[i][j]==1):
				if(adjMatrix[i][j]==0):
					print 'Invalid: Inexistent Link'
					return False #the link does not exist
				if(placementVector[j]!=1):
					print 'Invalid: Coupling Constraint'
					return False #it is connected to a controller that
					#does not exist
				connections = connections+1
		if(connections!=1):
			'Invalid: Connectivity Constraint'
			return False #each switch has one connection with a
			#controller
	totalLatency = 0
	for j in range(len(placementVector)):
		charge = 0
		for i in range(len(placementVector)):
			if(connectionMatrix[i][j]==1):
				totalLatency = totalLatency + latencyMatrix[i][j]
				charge = charge+sFreq[i]
		if(charge>cFreq):
			'Invalid: Overcharge Constraint'
			return False
		if(totalLatency>maxTotalLatency):
			'Invalid: Latency Constraint'
			return False
	return True

def evaluate(placementVector, connectionMatrix, costList, cPower, energy,\
	sProcEnergy,cProcEnergy,sFreq):
	tCost = 0
	
	#(static cost)
	for j in range(len(placementVector)):
		if(placementVector[j]==1):
			tCost = tCost+costList[j]*cPower
	
	#(cost due request processing)
	#for each switch
	for i in range(len(placementVector)):
		#for each possible location of a controller
		for j in range(len(placementVector)):
			#if the switch/controller edge exists
			if connectionMatrix[i][j]:
				#energy spent by the switch to process and send a request
				sEnergy = sProcEnergy[i]+energy[i][j]
				
				#energy spent by the controller to process and send a response
				cEnergy = cProcEnergy+energy[j][i]
				
				#total cost for the link i,j within 1 s
				tCost = tCost+(sEnergy*costList[i]
					+ cEnergy*costList[j])*sFreq[i]