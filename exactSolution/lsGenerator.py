#!/usr/bin/env python

import constraints
import objective

import sys
sys.path.insert(0,'../utils')

import inputParser
import inGen

def main():
	if(len(sys.argv)!=2):
		print 'Usage: lsGenerator [configuration file].xml'
		exit()

	inst = inputParser.eCPPInstance(sys.argv[1])
	maxTotalLatency = inst.getMaxTotalLatency()
	adjMatrix, latencyMatrix, energy = inst.getAdjMatrices()
	sFreq, cFreq = inst.getFrequencies()
	costList = inst.getCostList()
	sProcEnergy,cProcEnergy = inst.getProcEnergy()
	cPower = inst.getStaticPower()
	
	nSamples = 10
	if(cFreq==-1):
		cFreq = inGen.genCFreq(adjMatrix,sFreq,nSamples)
	if(maxTotalLatency==-1):
		maxTotalLatency = inGen.genMaxTotalLatency(adjMatrix,\
			latencyMatrix,nSamples)
	
	objective.generateObjective(costList,cPower,adjMatrix,energy,\
				sProcEnergy,cProcEnergy,sFreq)
	
	constraints.generateConstraints(adjMatrix,sFreq,cFreq,\
				latencyMatrix,maxTotalLatency)
	
	print 'Binary'
	for i in range(len(adjMatrix)):
		for j in range(len(adjMatrix)):
			if(adjMatrix[i][j]==1):
				print('\te'+str(i)+'_'+str(j))
	
	for i in range(len(adjMatrix)):
		print('\tp'+str(i))
	
	print 'End'

main()
