import random

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
