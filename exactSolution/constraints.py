def generateConnectivityConstraints(adjMatrix):
	#for each switch
	for i in range(len(adjMatrix)):
		s='\tConnectivityConstr'+str(i)+':'
		first=True
		#for each possible location of a controller
		for j in range(len(adjMatrix[i])):
			#if the switch/controller edge exists
			if adjMatrix[i][j]:
				if first:
					first = False
				else:
					s = s+'+'
				#the name of each variable corresponds to
				#its coordinates
				s = s+' 1 e'+str(i)+'_'+str(j)+' '	
		#each switch connects itself to only one controller
		print (s+'= 1')

def generateOverchargeConstraints(adjMatrix,sf,cf):
	#for each controller
	for j in range(len(adjMatrix)):
		s='\tOverchargeConstr'+str(j)+':'
		first=True
		#for each switch (reminding it is a square mat.)
		for i in range(len(adjMatrix)):
			if adjMatrix[i][j]:
				if first:
					first = False
				else:
					s = s+'+'
				s = s+' '+str(sf[i])+' e'+str(i)+'_'+str(j)+' '
		#the sum of the requests' rates cannot exceed controller's limit
		print (s+'<= '+str(cf))

def generateLatencyConstraint(adjMatrix,latencyMatrix,maxTotalLatency):
	first = True
	s = '\tLatencyConstr: '
	#for each switch
	for i in range(len(adjMatrix)):
		#for each possible location of a controller
		for j in range(len(adjMatrix[i])):
			#if the switch/controller edge exists
			if adjMatrix[i][j]:
				if first:
					first = False
				else:
					s = s+'+'
				#the name of each variable corresponds to
				#its coordinates
				s = s+' '+str(latencyMatrix[i][j])+' e'+str(i)+'_'+str(j)+' '	
	#the worst case total latency must not exceed a value
	print (s+'<= '+str(maxTotalLatency))

#couple the connection variables and the placement variables
def generateCouplingConstraints(adjMatrix):
	#for each controller
	for j in range(len(adjMatrix)):
		s='\tOverchargeConstr'+str(j)+':'
		first=True
		#for each switch (reminding it is a square mat.)
		for i in range(len(adjMatrix)):
			if adjMatrix[i][j]:
				if first:
					first = False
				else:
					s = s+'+'
				s = s+' e'+str(i)+'_'+str(j)+' '
		#if p_j is zero, there is no controller at this location, so
		#no connection is allowed. Otherwise, the maximum number of
		#connections is one for each switch
		print (s+'- '+str(len(adjMatrix))+' p'+str(j)+' <= 0')

def generateConstraints(adjMatrix,sfreq,cfreq,latencyMatrix,maxTotalLatency):
	print 'Subject to'
	generateConnectivityConstraints(adjMatrix)
	generateOverchargeConstraints(adjMatrix,sfreq,cfreq)
	generateLatencyConstraint(adjMatrix,latencyMatrix,maxTotalLatency)
	generateCouplingConstraints(adjMatrix)
