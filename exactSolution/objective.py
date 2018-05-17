def constCost(costList):
	s=str(costList[0])+' p0'
	for i in range(1,len(costList)):
		s=s+' + '+str(costList[i])+' p'+str(i)
	return s

def varCost(costList,totalEnergyMatrix,adjMatrix):
	s = ''
	#for each switch
	for i in range(len(adjMatrix)):
		#for each possible location of a controller
		for j in range(len(adjMatrix[i])):
			#if the switch/controller edge exists
			if adjMatrix[i][j]:
				#the name of each variable corresponds to
				#its coordinates.
				s = s+' '+str(totalEnergyMatrix[i][j]*costList[j])\
					+' e'+str(i)+'_'+str(j)+' '	
	#the worst case total latency must not exceed a value
	return s

def generateObjective(costList,totalEnergyMatrix,adjMatrix):
	print 'Minimize'
	print ('\tobj: '+constCost(costList)+' + '+varCost(\
		costList,totalEnergyMatrix,adjMatrix))
