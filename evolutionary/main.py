#Evolutionary algorithm for solving the energy cost oriented
#controller placement problem

import random
from heapq import merge
import sys
sys.path.insert(0,'../utils')
import eCPPGraph
import evaluateSolution


class indiviual:
	def __init__(self,nNodes,connectionMatrix=[],fitness=float('inf')):
		if connectionMatrix==[]:
			self.connectionMatrix = [[0 for n in range(nNodes)]\
				for n in range(nNodes)]
		else:
			self.connectionMatrix = connectionMatrix
		self.fitness = fitness
	
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
	
	def evalutateFitness(self,graph):
		placementVector = self.getPlacementVector()
		self.fitness,_ = evaluateSolution.eval(placementVector,\
			self.connectionMatrix,graph)
	
	def fixInconsistencies(self,graph):
		#TODO
	
	#Functions to generate new valid individuals
	def fillRandomly(self,graph):
		self.connectionMatrix = [[0 for n in graph.nodeList]\
			for n in graph.nodeList]
		
		#generates a random controller+switch florest
		for i in range(len(graph.nodeList))
			e = random.choice(graph.nodeList[i]._neighborhood)
			self.connectionMatrix[i][e.toNode] = 1
		
		self.fixInconsistencies(graph)
		
		self.evaluateFitness(graph)
	
	def crossover(self,partner,pivot,graph):
		#switch the genome from the pivot
		aux = partner.connectionMatrix[pivot:]
		partner.connectionMatrix[pivot:] = self.connectionMatrix[pivot:]
		self.connectionMatrix[pivot:] = aux
		
		#fix any inconsistencies
		self.fixInconsistencies(graph)
		partner.fixInconsistencies(graph)
		
		self.evaluateFitness(graph)
	
	def mutation(self,graph):
		i = random.randint(0, len(graph.nodeList)-1)
		j = random.randint(0, len(graph.nodeList)-1)
		self.connectionMatrix[i][j] = 1-self.connectionMatrix[i][j]
		self.fixInconsistencies(graph)
		
		self.evaluateFitness(graph)

def copyPopulation(population):
	cpPopupation = []
	for i in population:
		connectionMatrix = i0.copyConnectionMatrix()
		cpPopulation.append(\
			individual(len(connectionMatrix),connectionMatrix,i0.fitness))
	return cpPopulation

def applyCrossover(newGen,crossoverProb,graph):
	#TODO

def applyMutation(newGen,mutationProb,graph):
	#TODO

def select(population):
	#TODO

def run(graph,nGenerations,mutationProb,crossoverProb):
	population = generateFirstPopulation(graph)
	population.sort(key=lambda item: item.fitness)
	
	for i in range(nGenerations):
		newGen = copyPopulation(population)
		
		#apply evolutive operators
		applyCrossover(newGen,crossoverProb,graph)
		newGen.sort(key=lambda item: item.fitness)
		applyMutation(newGen,mutationProb,graph)
		
		population = list(merge(population, newGen))

		best = population[0]
		
		population = select(population)
		
		#elitism
		if best not in population:
			population.append(best)
	
	return best.fitness, True, best.getPlacementVector(), best.connectionMatrix