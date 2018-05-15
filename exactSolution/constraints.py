#the function below eliminates the out-of-bounds-latency
#and returns the matrix of latency
def eliminateLargeLatency(adjMatrix,propTimeMatrix,\
	sProcTimeVector,cProcTime,maxLatency):
	latencyMatrix = [[0 for i in adjMatrix] for j in adjMatrix]
	#for each switch
	for i in range(len(adjMatrix)):
		#for each possible location of a controller
		for j in range(len(adjMatrix[i])):
			#if the switch/controller edge exists
			if adjMatrix[i][j]:
				latencyMatrix[i][j] = cProcTime+propTimeMatrix[i][j]\
					+sProcTimeVector[i]
				#if the latency exceeds the maximum
				if latencyMatrix[i][j]>maxLatency:
					#disconsider this edge
					adjMatrix[i][j]=0
	return latencyMatrix

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
				s = s+' 1 s'+str(i)+'_'+str(j)+' '	
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
				s = s+' '+str(sf[i])+' s'+str(i)+'_'+str(j)+' '
		#the sum of the requests' rates cannot exceed controller's limit
		print (s+'<= '+str(cf))
	
def generateLatencyConstraint(adjMatrix,latencyMatrix,maxTotalLatency):
	first = True
	s = '\nLatencyConstr: '
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
				s = s+' '+latencyMatrix[i][j]+' s'+str(i)+'_'+str(j)+' '	
	#the worst case total latency must not exceed a value
	print (s+'<= '+maxTotalLatency)

def generateConstraints(adjMatrix,propTimeMatrix,sProcTimeVector,\
	cProcTime,maxLatency,maxTotalLatency,sf,cf):
	latencyMatrix = eliminateLargeLatency(adjMatrix,\
					propTimeMatrix, sProcTimeVector,\
					cProcTime,maxLatency)
	print 'Subject to'
	generateConnectivityConstraint(adjMatrix)
	generateOverchargeConstraint(adjMatrix,sf,cf)
	generateLatencyConstraints(adjMatrix,latencyMatrix,maxTotalLatency)
