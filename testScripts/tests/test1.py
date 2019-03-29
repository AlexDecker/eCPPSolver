#!/usr/bin/env python

import sys
import time

sys.path.insert(0,'..')
import generateXMLInput

sys.path.insert(0,'../../utils')
import eCPPGraph

sys.path.insert(0,'../../exactSolution')
import exactSolution

sys.path.insert(0,'../../heuristic1')
import heuristic1

sys.path.insert(0,'../../heuristic2')
import heuristic2

sys.path.insert(0,'../../heuristic3')
import heuristic3

n = 5

end = False

while True:
	for l in [10,50,100]:
		for eDiv in [1,2]:
			for i in range(5):
				generateXMLInput.gen(nNodes=n,edgeDivisor=eDiv,\
						minLongitude=l,maxLongitude=l,\
						minLatitude =l,maxLatitude =l,\
						controllerResponseFrequency=-1)
				
				attempts=0
				while attempts<5:
					graph = eCPPGraph.graph('generatedInput.xml',nSamplesFreq=1,nSamplesLat=100)
					t0 = time.time()
					valueExact, feasible, _, _ = exactSolution.run(graph,timeout=300)
					t1 = time.time()
					if feasible:
						break
					elif feasible==None:
						attempts = attempts+1
					else:
						print '(Not feasible)'
				if attempts==5:
					end=True
				
				if end:
					break
				else:
					valueHeu1, _, _, _ = heuristic1.run(graph)
					t2 = time.time()
					valueHeu2, _, _, _ = heuristic2.run(graph)
					t3 = time.time()
					valueHeu3, _, _, _ = heuristic3.run(graph)
					t4 = time.time()
					out = str(n)+', '+str(l)+', '+str(eDiv)+', '+\
						str(valueExact)+', '+str(t1-t0)+', '+\
						str(valueHeu1)+', '+str(t2-t1)+', '+\
						str(valueHeu2)+', '+str(t3-t2)+', '+\
						str(valueHeu3)+', '+str(t4-t3)+';\n'
					print out
					#it opens, appends and closes the file every time in order
					#to preserve the data
					with open('experiments.log', 'a') as myfile:
						myfile.write(out)
						myfile.close()
			if end:
				break
		if end:
			break
	if end:
		break
	n = n+1
