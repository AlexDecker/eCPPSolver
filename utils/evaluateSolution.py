#this file validates some configuration of both
#placementVector (a binary vector that points what locations have a
#controller) and the connectionMatrix (a binary matrix that points if
#switch i has a control connection with the controller at location j)
#and evaluates its energy consumption (including constant values, so
#the value evaluated here will be greater than glpk's)

import eCPPGraph

def eval(placementVector, connectionMatrix, graph, verbose=False):
	f = evaluateFeasibility(placementVector, connectionMatrix,\
		graph.adjMatrix,graph.latencyMatrix,\
		graph.sFreq,graph.cFreq,graph.maxTotalLatency,verbose)
	v = evaluateValue(placementVector, connectionMatrix,\
		graph.costList, graph.cPower, graph.sPower,\
		graph.energy, graph.sProcEnergy, graph.cProcEnergy, graph.sFreq)
	return v, f

def evaluateFeasibility(placementVector, connectionMatrix, adjMatrix,\
	latencyMatrix,sFreq,cFreq,maxTotalLatency,verbose):
	for i in range(len(placementVector)):
		connections = 0
		for j in range(len(placementVector)):
			if(connectionMatrix[i][j]==1):
				if(adjMatrix[i][j]==0):
					if verbose:
						print 'Invalid: Inexistent Link'
					return False #the link does not exist
				if(placementVector[j]!=1):
					if verbose:
						print 'Invalid: Coupling Constraint'
					return False #it is connected to a controller that
					#does not exist
				connections = connections+1
		if(connections!=1):
			if verbose:
				print 'Invalid: Connectivity Constraint'
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
			if verbose:
				print 'Invalid: Overcharge Constraint'
			return False
		if(totalLatency>maxTotalLatency+0.00000001):
			if verbose:
				print 'Invalid: Latency Constraint: '+str(totalLatency)+', '+str(maxTotalLatency)
			return False
	return True

def evaluateValue(placementVector, connectionMatrix, costList, cPower, sPower,\
	energy,sProcEnergy,cProcEnergy,sFreq):
	tCost = 0
	
	#(static cost)
	for j in range(len(placementVector)):
		tCost = tCost+costList[j]*sPower[j]#switch
		if(placementVector[j]==1):
			tCost = tCost+costList[j]*cPower#controller
	x = tCost
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
				tCost = tCost+(sEnergy*costList[i]\
					+ cEnergy*costList[j])*sFreq[i]
	if tCost!=0:
		print ('How much is spent with static costs: '+str(100*x/tCost)+'%')
	return tCost
