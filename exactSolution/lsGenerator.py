#!/usr/bin/env python

import constraints
import objective

import sys
sys.path.insert(0,'../utils')

import inputParser

def main():
	if(len(sys.argv)!=2):
		print 'Usage: lsGenerator [configuration file].xml'
		exit()

	inst = inputParser.eCPPInstance(sys.argv[1])
	maxTotalLatency = inst.getMaxTotalLatency()
	adjMatrix, latencyMatrix, totalEnergyMatrix = inst.getAdjMatrices()
	sfreq, cfreq = inst.getFrequencies()
	costList = inst.getCostList()
	
	objective.generateObjective(costList,totalEnergyMatrix,adjMatrix)
	
	constraints.generateConstraints(adjMatrix,sfreq,cfreq,\
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
