#(antes estava minEnergyPerBitPerKm = 0.00001, maxEnergyPerBitPerKm = 0.0001,\)
#generates a XML file for eCPPGraph
def gen(filename='generatedInput.xml',\
	nNodes = 13, edgeDivisor=1,\
	minLongitude = 10, maxLongitude = 20,\
	minLatitude = 10, maxLatitude = 20,\
	minLightSpeed = 150000, maxLightSpeed = 170000,\
	minControllerPacketProcessingTime = 0.001,\
	maxControllerPacketProcessingTime = 0.003,\
    minSwitchPacketProcessingTime = 0.0005,\
    maxSwitchPacketProcessingTime =  0.001,\
	minEnergyPerBitPerKm = 0.00001, maxEnergyPerBitPerKm = 0.00001,\
	avgMsgSize = 12000, minSwitchRequestFrequency = 416667,\
	maxSwitchRequestFrequency = 833333,\
	controllerResponseFrequency=-1,\
	minCost = 0.0007, maxCost = 0.0017,\
	minControllerStaticPower = 400, maxControllerStaticPower = 600,\
	minControllerPacketProcessingEnergy = 0.005,\
	maxControllerPacketProcessingEnergy = 0.007,\
	minSwitchStaticPower = 200, maxSwitchStaticPower = 300,\
	minSwitchPacketProcessingEnergy = 0.001,\
	maxSwitchPacketProcessingEnergy = 0.003):
	
	m = int((nNodes**2-nNodes)/(2*edgeDivisor))

	if((nNodes<2)or(edgeDivisor<1)or(m<nNodes-1)):
		print 'generateXMLInput: Input error'
		exit()

	xml =   ('<settings\n\
		    autoGen="True"\n\
		    nNodes="'+str(nNodes)+'"\n\
		    nEdges="'+str(m)+'"\n\
		    minLongitude="'+str(minLongitude)+'"\n\
		    maxLongitude="'+str(maxLongitude)+'"\n\
		    minLatitude ="'+str(minLatitude)+'"\n\
		    maxLatitude ="'+str(maxLatitude)+'"\n\
		    minLightSpeed = "'+str(minLightSpeed)+'"\n\
		    maxLightSpeed = "'+str(maxLightSpeed)+'"\n\
		    minControllerPacketProcessingTime = "'+str(minControllerPacketProcessingTime)+'"\n\
		    maxControllerPacketProcessingTime = "'+str(maxControllerPacketProcessingTime)+'"\n\
		    minSwitchPacketProcessingTime = "'+str(minSwitchPacketProcessingTime)+'"\n\
		    maxSwitchPacketProcessingTime = "'+str(maxSwitchPacketProcessingTime)+'"\n\
		    minEnergyPerBitPerKm = "'+str(minEnergyPerBitPerKm)+'"\n\
		    maxEnergyPerBitPerKm = "'+str(maxEnergyPerBitPerKm)+'"\n\
		    avgMsgSize = "'+str(avgMsgSize)+'"\n\
		    minSwitchRequestFrequency = "'+str(minSwitchRequestFrequency)+'"\n\
		    maxSwitchRequestFrequency = "'+str(maxSwitchRequestFrequency)+'"\n\
		    controllerResponseFrequency = "'+str(controllerResponseFrequency)+'"\n\
		    minCost = "'+str(minCost)+'"\n\
		    maxCost = "'+str(maxCost)+'"\n\
		    minControllerStaticPower = "'+str(minControllerStaticPower)+'"\n\
		    maxControllerStaticPower = "'+str(maxControllerStaticPower)+'"\n\
		    minControllerPacketProcessingEnergy = "'+str(minControllerPacketProcessingEnergy)+'"\n\
		    maxControllerPacketProcessingEnergy = "'+str(maxControllerPacketProcessingEnergy)+'"\n\
		    minSwitchStaticPower = "'+str(minSwitchStaticPower)+'"\n\
			maxSwitchStaticPower = "'+str(maxSwitchStaticPower)+'"\n\
		    minSwitchPacketProcessingEnergy = "'+str(minSwitchPacketProcessingEnergy)+'"\n\
		    maxSwitchPacketProcessingEnergy = "'+str(maxSwitchPacketProcessingEnergy)+'"\n\
			></settings>')
	
	with open(filename, 'w') as myfile:
		myfile.write(xml)
		myfile.close()
