#!/usr/bin/env python

import sys

if(len(sys.argv)!=3):
	print './testExactInGen [nNodes] [edgeDivisor]'
	exit()
n = int(sys.argv[1])
eDiv = float(sys.argv[2])
m = int((n**2-n)/(2*eDiv))

if((n<2)or(eDiv<1)or(m<n-1)):
	print 'Input error'
	exit()

print  '<settings\n\
        autoGen="True"\n\
        nNodes="'+sys.argv[1]+'"\n\
        nEdges="'+str(m)+'"\n\
        minLongitude="10"\n\
        maxLongitude="20"\n\
        minLatitude ="10"\n\
        maxLatitude ="20"\n\
        minLightSpeed = "150000"\n\
        maxLightSpeed = "170000"\n\
        minControllerPacketProcessingTime = "0.001"\n\
        maxControllerPacketProcessingTime = "0.003"\n\
        minSwitchPacketProcessingTime = "0.0005"\n\
        maxSwitchPacketProcessingTime = "0.001"\n\
        minEnergyPerBitPerKm = "0.00001"\n\
        maxEnergyPerBitPerKm = "0.0001"\n\
        avgMsgSize = "128"\n\
        minSwitchRequestFrequency = "100"\n\
        maxSwitchRequestFrequency = "200"\n\
        minCost = "0.0007"\n\
        maxCost = "0.0017"\n\
        minControllerStaticPower = "400"\n\
        maxControllerStaticPower = "600"\n\
        minControllerPacketProcessingEnergy = "0.005"\n\
        maxControllerPacketProcessingEnergy = "0.007"\n\
        minSwitchPacketProcessingEnergy = "0.001"\n\
        maxSwitchPacketProcessingEnergy = "0.003"\n\
		></settings>'
