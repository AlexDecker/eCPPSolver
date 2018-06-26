#Evolutionary algorithm for solving the energy cost oriented
#controller placement problem

import random
import math
from heapq import merge
import sys
sys.path.insert(0,'../utils')
import eCPPGraph
import evaluateSolution


class indiviual:
	def __init__(self,nNodes,connectionMatrix=[],invFitness=float('inf')):
		if connectionMatrix==[]:
			self.connectionMatrix = [[0 for n in range(nNodes)]\
				for n in range(nNodes)]
		else:
			self.connectionMatrix = connectionMatrix
		self.invFitness = invFitness
	
	def copyConnectionMatrix(self):
		return ([\
			[self.connectionMatrix[i][j] for i in range(len(self.connectionMatrix))]\
			for n in  range(len(self.connectionMatrix))\
			])
	
	def getPlacementVector(self):
		placementVector = []
		for i in range(len(self.connectonMatrix)):
			placementVector.append(self.connectonMatrix[i][i])
		return placementVector
	
	#the individual is better if its inverse fitness is smaller
	def evalutateInvFitness(self,graph):
		placementVector = self.getPlacementVector()
		self.invFitness,_ = evaluateSolution.eval(placementVector,\
			self.connectionMatrix,graph)
	
	def fixInconsistencies(self,graph):
		for j in range(0,self.connectionMatrix):
			for i in range(0,self.connectionMatrix):
				
	
	#Functions to generate new valid individuals
	def fillRandomly(self,graph):
		self.connectionMatrix = [[0 for n in graph.nodeList]\
			for n in graph.nodeList]
		
		#generates a random controller+switch florest
		for i in range(len(graph.nodeList))
			e = random.choice(graph.nodeList[i]._neighborhood)
			self.connectionMatrix[i][e.toNode] = 1
		
		self.fixInconsistencies(graph)
		
		self.evaluateInvFitness(graph)
	
	def crossover(self,partner,pivot,graph):
		#switch the genome from the pivot
		aux = partner.connectionMatrix[pivot:]
		partner.connectionMatrix[pivot:] = self.connectionMatrix[pivot:]
		self.connectionMatrix[pivot:] = aux
		
		#fix any inconsistencies
		self.fixInconsistencies(graph)
		partner.fixInconsistencies(graph)
		
		self.evaluateInvFitness(graph)
		partner.evaluateInvFitness(graph)
	
	def mutation(self,graph):
		i = random.randint(0, len(graph.nodeList)-1)
		j = random.randint(0, len(graph.nodeList)-1)
		self.connectionMatrix[i][j] = 1-self.connectionMatrix[i][j]
		self.fixInconsistencies(graph)
		
		self.evaluateInvFitness(graph)
	
def select(population,nIndividuals,sumInvFitness):
	while len(population)>nIndividuals:
		chosen = random.uniform(0, sumInvFitness)
		acc = 0
		for ind in population:
			if (chosen>=acc) and (chosen<=acc+ind.invFitness):
				sumInvFitness = sumInvFitness - ind.invFitness
				population.remove(ind)
				break
			acc = acc + ind.invFitness

def copyPopulation(population):
	newPop = []
	for ind in pop:
		connectionMatrix = ind.copyConnectionMatrix()
		newInd = individual(len(connectionMatrix), connectionMatrix,ind.fitness)
		newPop.append(newInd)
	return newPop

def run(graph,nIndividuals,nGenerations,mutationRate,crossoverRate):
	if(len(graph.nodeList)<2):
		print 'It must have at least two nodes'
		exit()
	
	population,sumInvFitness = generateFirstPopulation(graph,nIndividuals)
	
	for i in range(nGenerations):
		#apply evolutive operators
		
		#mutation
		pop = random.sample(newGen,mutationRate*len(population))
		mutants = copyPopulation(pop)
		for ind in mutants:
			ind.mutation(graph)
		
		#crossover
		pop = list(population)
		nCouples = math.floor(crossoverRate*len(population)/2)
		#eliminate the worst<=> select the best
		select(pop,2*nCouples,sumInvFitness)
		couples = copyPopulation(pop)
		for i in range(0,2*nCouples,2):
			couples[i].crossover(couples[i+1],random.randint(1, len(graph.nodeList)-1),graph)
		
		population = population + couples + mutants

		best = min(population, key=lambda x: x.invFitness)
		
		population = select(population,nIndividuals)
		
		#elitism
		if best not in population:
			population.append(best)
	
	return best.invFitness, True, best.getPlacementVector(), best.connectionMatrix
