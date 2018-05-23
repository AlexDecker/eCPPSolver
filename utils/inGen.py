import random
from math import radians, cos, sin, asin, sqrt

#the matrix generated has always the main diagonal with ones
#and is symmetric. So, m is the number of desired edges
#in the triangular sub-matriz, not including the main diagonal,
#therefore being limited to (n2-n)/2. As the matrix must have
#connectivity, m is also limited by n-1
def genRandomAdjMatrix(n,m):
	#generating an auxiliar vector with the desired number of edges
	auxVector = [0 for i in range((n*n-n)/2)]
	for i in range(m):
		auxVector[i]=1
	#randomizing the generated vector
	random.shuffle(auxVector)
	#initializing and putting the ones in the main diagonal 
	adjMatrix = [[0 for j in range(n)]for i in range(n)]
	for i in range(n):
		adjMatrix[i][i] = 1
	#this vector associates each node to its number of connections
	connections = [0 for i in range(n)]
	#placing the values in the right place
	k=0
	for i in range(n-1):
		for j in range(i+1,n):
			adjMatrix[i][j] = auxVector[k]
			if(adjMatrix[i][j]==1):
				connections[i] = connections[i]+1
				connections[j] = connections[j]+1
			k = k+1
	#solving connectivity issues
	while True:
		valid = False
		for k in range(n):
			if(connections[k]==0):
				valid=True
				break
		if(not valid):#there is no vertex with no connection
			break
		else:
			#search for a link to remove in order to insert another
			#and reconnect the graph
			valid=False
			for i in range(n-1):
				for j in range(i+1,n):
					if((connections[i]>1)and(connections[j]>1)):
						adjMatrix[i][j] = 0 #remove this link
						connections[i] = connections[i]-1
						connections[j] = connections[j]-1
						valid=True
						break
				if(valid):#if something was found, break
					break
			if(not valid):#if not found
				print 'Connectivity not achieved'
				break
			#if found
			if(k!=n-1):
				l = random.randint(k+1,n-1)
				adjMatrix[k][l] = 1
				connections[k] = connections[k]+1
				connections[l] = connections[l]+1
			else:
				l = random.randint(0,n-1)
				adjMatrix[l][k] = 1
				connections[k] = connections[k]+1
				connections[l] = connections[l]+1
	
	#making the matrix symmetric
	for i in range(n-1):
		for j in range(i+1,n):
			if(adjMatrix[i][j]==1):
				adjMatrix[j][i]=1
	
	return adjMatrix
##################################################################################
#generates a cFreq value in order to ensure at least one solution
#(len(sFreq) must be equals to len(adjMatrix) and adjMatrix must be
#square)
def genCFreq_simple(adjMatrix,sFreq):
	#total frequency of requests in each controller
	chargeVec = [0 for i in range(len(sFreq))]
	for i in range(len(sFreq)):
		adjList = []
		for j in range(len(sFreq)):
			if(adjMatrix[i][j]==1):
				adjList.append(j)
		#choose a random controller to connect
		k = random.choice(adjList)
		#enlarge the charge over the corresponding controller
		chargeVec[k] = chargeVec[k]+sFreq[i]
	#return the limit that allows the random generated connections
	# to be valid
	return max(chargeVec)
		
#generates a cFreq value in order to ensure at least one solution
#(min of n random samples)
#(len(sFreq) must be equals to len(adjMatrix) and adjMatrix must be
#square)
def genCFreq(adjMatrix,sFreq,nSamples):
	samples = []
	for i in range(nSamples):
		samples.append(genCFreq_simple(adjMatrix,sFreq))
	return min(samples)
##################################################################################
#generates a maxTotalLatency value in order to ensure at least one solution
#(the dimensions of both matrices must be the same)
def genMaxTotalLatency_simple(adjMatrix,latencyMatrix):
	maxTotalLatency = 0
	for i in range(len(adjMatrix)):
		adjList = []
		for j in range(len(adjMatrix)):
			if(adjMatrix[i][j]==1):
				adjList.append(j)
		#choose a random controller to connect
		k = random.choice(adjList)
		#enlarge the total latency
		maxTotalLatency = maxTotalLatency + latencyMatrix[i][k]
	return maxTotalLatency

#The same of the last function, but now returning the min of n samples
def genMaxTotalLatency(adjMatrix,latencyMatrix,nSamples):
	samples = []
	for i in range(nSamples):
		samples.append(genMaxTotalLatency_simple(adjMatrix,latencyMatrix))
	return min(samples)
##################################################################################
#haversine formula (distance between geographical coordinates)
def haversine(lon1, lat1, lon2, lat2):
    # convert decimal degrees to radians 
    lon1, lat1, lon2, lat2 = map(radians, [lon1, lat1, lon2, lat2])

    # haversine formula (https://en.wikipedia.org/wiki/Haversine_formula)
    dlon = lon2 - lon1 
    dlat = lat2 - lat1 
    a = sin(dlat/2)**2 + cos(lat1) * cos(lat2) * sin(dlon/2)**2
    c = 2 * asin(sqrt(a)) 
    r = 6371 # Radius of earth in kilometers
    return c * r

#generates a random placement for the possible locations
def genRandomDistMatrix(adjMatrix,minLon,maxLon,minLat,maxLat):
	latitude = []
	longitude = []
	for i in range(len(adjMatrix)):
		latitude.append(random.uniform(minLat,maxLat))
		longitude.append(random.uniform(minLon,maxLon))
	distMatrix = [[0 for i in range(len(adjMatrix))] for j in range(len(adjMatrix))]
	for i in range(len(adjMatrix)):
		for j in range(len(adjMatrix[i])):
			if(adjMatrix[i][j]==1):
				distMatrix[i][j]=haversine(longitude[i],latitude[i],\
					longitude[j],latitude[j])
	return distMatrix
##################################################################################
#cMatrix is the velocity of the light in each link
def genPropTimeMatrix(cMatrix,distMatrix):
	propTimeMatrix = [[0 for i in range(len(distMatrix))]\
		for j in range(len(distMatrix))]
	
	for i in range(len(propTimeMatrix)):
		for j in range(len(propTimeMatrix[i])):
			propTimeMatrix[i][j] = distMatrix[i][j]/cMatrix[i][j]
	return propTimeMatrix

def genLatencyMatrix(cMatrix,distMatrix,sProcTime,cProcTime):
	latencyMatrix = genPropTimeMatrix(cMatrix,distMatrix)
	for i in range(len(latencyMatrix)):
		for j in range(len(latencyMatrix[i])):
			latencyMatrix[i][j] = 2*latencyMatrix[i][j]+sProcTime[i]+cProcTime
	return latencyMatrix
##################################################################################
#calculates the energy needed to send a message through each link
#eMatrix is energy(J) per bit per km in each link. avgMsgSize is in bits
def genEnergyMatrix(eMatrix,distMatrix,avgMsgSize):
	energyMatrix = [[0 for i in range(len(distMatrix))]\
		for j in range(len(distMatrix))]
	for i in range(len(energyMatrix)):
		for j in range(len(energyMatrix[i])):
			energyMatrix[i][j] = energyMatrix[i][j]*eMatrix[i][j]*avgMsgSize
	return energyMatrix
